#include "flex_typeclass_plugin/Tooling.hpp" // IWYU pragma: associated
#include "flex_typeclass_plugin/flex_typeclass_plugin_settings.hpp"

#include <flexlib/per_plugin_settings.hpp>
#include <flexlib/reflect/ReflTypes.hpp>
#include <flexlib/reflect/ReflectAST.hpp>
#include <flexlib/reflect/ReflectionCache.hpp>
#include <flexlib/ToolPlugin.hpp>
#include <flexlib/core/errors/errors.hpp>
#include <flexlib/utils.hpp>
#include <flexlib/funcParser.hpp>
#include <flexlib/inputThread.hpp>
#include <flexlib/clangUtils.hpp>
#include <flexlib/clangPipeline.hpp>
#include <flexlib/annotation_parser.hpp>
#include <flexlib/annotation_match_handler.hpp>
#include <flexlib/matchers/annotation_matcher.hpp>
#include <flexlib/options/ctp/options.hpp>
#if defined(CLING_IS_ON)
#include "flexlib/ClingInterpreterModule.hpp"
#endif // CLING_IS_ON

#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Preprocessor.h>

#include <base/cpu.h>
#include <base/bind.h>
#include <base/command_line.h>
#include <base/debug/alias.h>
#include <base/debug/stack_trace.h>
#include <base/memory/ptr_util.h>
#include <base/sequenced_task_runner.h>
#include <base/strings/string_util.h>
#include <base/trace_event/trace_event.h>
#include <base/logging.h>
#include <base/files/file_util.h>
#include <base/path_service.h>

#include <any>
#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>

namespace plugin {

TypeclassTooling::TypeclassTooling(
  const ::plugin::ToolPlugin::Events::RegisterAnnotationMethods& event
#if defined(CLING_IS_ON)
  , ::cling_utils::ClingInterpreter* clingInterpreter
#endif // CLING_IS_ON
) : clingInterpreter_(clingInterpreter)
{
  DCHECK(clingInterpreter_)
    << "clingInterpreter_";

  // load settings from C++ script interpreted by Cling
  /// \note skip on fail of settings loading,
  /// fallback to defaults
  {
    cling::Value clingResult;
    /**
     * EXAMPLE Cling script:
       namespace flex_typeclass_plugin {
         // Declaration must match plugin version.
         struct Settings {
           // output directory for generated files
           std::string outDir;
         };
         void loadSettings(Settings& settings)
         {
           settings.outDir
             = "${flextool_outdir}";
         }
       } // namespace flex_typeclass_plugin
     */
    cling::Interpreter::CompilationResult compilationResult
      = clingInterpreter_->callFunctionByName(
          // function name
          "flex_typeclass_plugin::loadSettings"
          // argument as void
          , static_cast<void*>(&settings_)
          // code to cast argument from void
          , "*(flex_typeclass_plugin::Settings*)"
          , clingResult);
    if(compilationResult
        != cling::Interpreter::Interpreter::kSuccess) {
      DCHECK(settings_.outDir.empty());
      DVLOG(9)
        << "failed to execute Cling script, "
           "skipping...";
    } else {
      DVLOG(9)
        << "settings_.outDir: "
        << settings_.outDir;
    }
    DCHECK(clingResult.hasValue()
      // we expect |void| as result of function call
      ? clingResult.isValid() && clingResult.isVoid()
      // skip on fail of settings loading
      : true);
  }

  DETACH_FROM_SEQUENCE(sequence_checker_);

  DCHECK(event.sourceTransformPipeline)
    << "event.sourceTransformPipeline";
  ::clang_utils::SourceTransformPipeline& sourceTransformPipeline
    = *event.sourceTransformPipeline;

  sourceTransformRules_
    = &sourceTransformPipeline.sourceTransformRules;

  if (!base::PathService::Get(base::DIR_EXE, &dir_exe_)) {
    NOTREACHED();
  }
  DCHECK(!dir_exe_.empty());

  outDir_ = dir_exe_.Append("generated");

  if(!settings_.outDir.empty()) {
     outDir_ = base::FilePath{settings_.outDir};
  }

  if(!base::PathExists(outDir_)) {
    base::File::Error dirError = base::File::FILE_OK;
    // Returns 'true' on successful creation,
    // or if the directory already exists
    const bool dirCreated
      = base::CreateDirectoryAndGetError(outDir_, &dirError);
    if (!dirCreated) {
      LOG(ERROR)
        << "failed to create directory: "
        << outDir_
        << " with error code "
        << dirError
        << " with error string "
        << base::File::ErrorToString(dirError);
    }
  }

  {
    // Returns an empty path on error.
    // On POSIX, this function fails if the path does not exist.
    outDir_ = base::MakeAbsoluteFilePath(outDir_);
    DCHECK(!outDir_.empty());
    VLOG(9)
      << "outDir_= "
      << outDir_;
  }
}

TypeclassTooling::~TypeclassTooling()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

clang_utils::SourceTransformResult
  TypeclassTooling::typeclass(
    const clang_utils::SourceTransformOptions& sourceTransformOptions)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(9)
    << "typeclass called...";

  // used by code generator of inline typeclass
  InlineTypeclassSettings inlineTypeclassSettings;

  flexlib::args typeclassBaseNames =
    sourceTransformOptions.func_with_args.parsed_func_.args_;

  clang::SourceManager &SM
    = sourceTransformOptions.rewriter.getSourceMgr();

  const clang::CXXRecordDecl *node1 =
    sourceTransformOptions.matchResult.Nodes.getNodeAs<
      clang::CXXRecordDecl>("bind_gen");

  if (!node1) {
    LOG(ERROR)
      << "CXXRecordDecl not found ";
    CHECK(false);
    return clang_utils::SourceTransformResult{nullptr};
  }

  /// \todo support custom namespaces
  reflection::NamespacesTree m_namespaces;

  std::string fullBaseType;

  if (node1) {
    reflection::AstReflector reflector(
      sourceTransformOptions.matchResult.Context);

    std::vector<clang::CXXRecordDecl *> nodes;
    std::vector<reflection::ClassInfoPtr> structInfos;
    reflection::ClassInfoPtr structInfo;

    for(const clang::CXXBaseSpecifier& it
        : node1->bases())
    {
      if(it.getType()->getAsCXXRecordDecl()) {
        nodes.push_back(
          it.getType()->getAsCXXRecordDecl());

        const auto refled
          = reflector.ReflectClass(
            it.getType()->getAsCXXRecordDecl(),
            &m_namespaces, false);

        structInfos.push_back(refled);

        if(!structInfo) {
          structInfo = refled;
        } else {
          for(const auto& it : refled->members) {
            structInfo->members.push_back(it);
          }
          for(const auto& it : refled->methods) {
            structInfo->methods.push_back(it);
          }
          for(const auto& it : refled->innerDecls) {
            structInfo->innerDecls.push_back(it);
          }
        }

        std::string preparedFullBaseType
          = it.getType().getAsString();
        preparedFullBaseType
          = exatractTypeName(preparedFullBaseType);

        structInfo->compoundId.push_back(
          preparedFullBaseType);

        fullBaseType += preparedFullBaseType;
        fullBaseType += ",";
      }
    }

    // remove last comma
    if (!fullBaseType.empty()) {
      fullBaseType.pop_back();
    }

    if(nodes.empty() || structInfos.empty()
       || !structInfo) {
      CHECK(false);
      return clang_utils::SourceTransformResult{""};
    }

    std::string targetTypeName;
    std::string targetGenerator;
    for(const auto& tit : typeclassBaseNames.as_vec_) {
      if(tit.name_ == "name")
      {
        targetTypeName = tit.value_;
        prepareTplArg(targetTypeName);
        DCHECK(!targetTypeName.empty());
      }
      else if(tit.name_ == "generator")
      {
        targetGenerator = tit.value_;
        prepareTplArg(targetGenerator);
        DCHECK(targetGenerator == "InPlace"
          || targetGenerator == "InHeap");
      }
      else if(tit.name_ == "BufferSize")
      {
        inlineTypeclassSettings.BufferSize
          = tit.value_;
        prepareTplArg(inlineTypeclassSettings.BufferSize);
        DCHECK(!inlineTypeclassSettings.BufferSize.empty());
      }
      else if(tit.name_ == "BufferAlignment")
      {
        inlineTypeclassSettings.BufferAlignment
          = tit.value_;
        prepareTplArg(inlineTypeclassSettings.BufferAlignment);
        DCHECK(!inlineTypeclassSettings.BufferAlignment.empty());
      }
    }

    VLOG(9)
     << "targetTypeName: "
     << targetTypeName;

    {
      std::string registryTargetName =
        targetTypeName.empty()
        ? fullBaseType
        : targetTypeName;

      VLOG(9) << "ReflectionRegistry... for record " <<
        registryTargetName;

      reflection::ReflectionRegistry::getInstance()->
        reflectionCXXRecordRegistry[registryTargetName]
          = std::make_unique<
              reflection::ReflectionCXXRecordRegistry>(
                registryTargetName, /*node,*/ structInfo);

      VLOG(9) << "registering type for trait...";

      traitToItsType_[registryTargetName]
        = fullBaseType;

      /// \todo template support
      DCHECK(!structInfo->templateParams.size());

      VLOG(9) << "templateParams.size"
        << structInfo->templateParams.size();
      VLOG(9) << "genericParts.size"
        << structInfo->genericParts.size();

      std::string fileTargetName =
        targetTypeName.empty()
        ? fullBaseType
        : targetTypeName;

      normalizeFileName(fileTargetName);

      base::FilePath gen_hpp_name
        = outDir_.Append(fileTargetName + ".typeclass.generated.hpp");

      base::FilePath gen_cpp_name
        = outDir_.Append(fileTargetName + ".typeclass.generated.cpp");

      {
        std::string headerGuard = "";

        std::string generator_path
          = TYPECLASS_TEMPLATE_HPP;
        DCHECK(!generator_path.empty())
          << TYPECLASS_TEMPLATE_HPP
          << "generator_path.empty()";

        const auto fileID = SM.getMainFileID();
        const auto fileEntry = SM.getFileEntryForID(
          SM.getMainFileID());
        std::string full_file_path = fileEntry->getName();

        std::vector<std::string> generator_includes{
             buildIncludeDirective(
              full_file_path),
             buildIncludeDirective(
              R"raw(type_erasure_common.hpp)raw")
          };

        reflection::ClassInfoPtr ReflectedStructInfo
          = structInfo;

        // squarets will generate code from template file
        // and append it after annotated variable
        /// \note FILE_PATH defined by CMakeLists
        /// and passed to flextool via
        /// --extra-arg=-DFILE_PATH=...
        _squaretsFile(
          TYPECLASS_TEMPLATE_HPP
        )
        std::string squarets_output = "";

        {
          DCHECK(squarets_output.size());
          const int writeResult = base::WriteFile(
            gen_hpp_name
            , squarets_output.c_str()
            , squarets_output.size());
          // Returns the number of bytes written, or -1 on error.
          if(writeResult == -1) {
            LOG(ERROR)
              << "Unable to write file: "
              << gen_hpp_name;
          } else {
            LOG(INFO)
              << "saved file: "
              << gen_hpp_name;
          }
        }

      }

      {
        std::string headerGuard = "";

        std::string generator_path
          = TYPECLASS_TEMPLATE_CPP;
        DCHECK(!generator_path.empty())
          << TYPECLASS_TEMPLATE_CPP
          << "generator_path.empty()";

        reflection::ClassInfoPtr ReflectedStructInfo
          = structInfo;

        const auto fileID = SM.getMainFileID();
        const auto fileEntry = SM.getFileEntryForID(
          SM.getMainFileID());
        std::string full_file_path = fileEntry->getName();

        std::vector<std::string> generator_includes{
             buildIncludeDirective(
              /// \todo utf8 support
              gen_hpp_name.AsUTF8Unsafe()),
             buildIncludeDirective(
              R"raw(type_erasure_common.hpp)raw")
          };

        // squarets will generate code from template file
        // and append it after annotated variable
        /// \note FILE_PATH defined by CMakeLists
        /// and passed to flextool via
        /// --extra-arg=-DFILE_PATH=...
        _squaretsFile(
          TYPECLASS_TEMPLATE_CPP
        )
        std::string squarets_output = "";

        {
          DCHECK(squarets_output.size());
          const int writeResult = base::WriteFile(
            gen_cpp_name
            , squarets_output.c_str()
            , squarets_output.size());
          // Returns the number of bytes written, or -1 on error.
          if(writeResult == -1) {
            LOG(ERROR)
              << "Unable to write file: "
              << gen_cpp_name;
          } else {
            LOG(INFO)
              << "saved file: "
              << gen_cpp_name;
          }
        }

      }

    }
  }

  return clang_utils::SourceTransformResult{nullptr};
}

clang_utils::SourceTransformResult
  TypeclassTooling::typeclass_instance(
    const clang_utils::SourceTransformOptions& sourceTransformOptions)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(9)
    << "typeclass_instance called...";

  // EXAMPLE:
  // _typeclass_impl(
  //   typeclass_instance(target = "FireSpell", "Printable")
  // )
  // Here |typeclassBaseNames| will contain:
  // (target = "FireSpell", "Printable")
  flexlib::args typeclassBaseNames =
    sourceTransformOptions.func_with_args.parsed_func_.args_;

  clang::SourceManager &SM
    = sourceTransformOptions.rewriter.getSourceMgr();

  std::string targetName;

  const clang::CXXRecordDecl *node =
    sourceTransformOptions.matchResult.Nodes.getNodeAs<
      clang::CXXRecordDecl>("bind_gen");

  if (node) {
    targetName = node->getNameAsString();
  }

  for(const auto& tit : typeclassBaseNames.as_vec_) {
    if(tit.name_ =="impl_target") {
      targetName = tit.value_;
      prepareTplArg(targetName);
    }
  }

  if (targetName.empty()) {
      LOG(ERROR)
        << "target for typeclass instance not found ";
      CHECK(false);
      return clang_utils::SourceTransformResult{nullptr};
  }

  std::string typeAlias;
  for(const auto& tit : typeclassBaseNames.as_vec_) {
    if(tit.name_ == "trait_type") {
      typeAlias = tit.value_;
      prepareTplArg(typeAlias);
    }
  }

  size_t processedTimes = 0;

  /// \todo remove loop
  for(const auto& tit : typeclassBaseNames.as_vec_) {
    VLOG(9)
      << "arg name: "
      << tit.name_
      << " arg value: "
      << tit.value_;

    if(tit.name_ == "impl_target") {
      continue;
    }

    if(tit.name_ == "trait_type") {
      continue;
    }

    processedTimes++;

    std::string typeclassBaseName = tit.value_;
    prepareTplArg(typeclassBaseName);

    VLOG(9)
      << "typeclassBaseName = "
      << typeclassBaseName;

    VLOG(9)
      << "target = "
      << targetName;

    if(typeclassBaseName.empty()) {
        return clang_utils::SourceTransformResult{""};
        CHECK(false);
    }

    std::string validTypeAlias = typeAlias;
    if(std::map<std::string, std::string>
        ::const_iterator it
          = traitToItsType_.find(typeclassBaseName)
       ; it != traitToItsType_.end())
    {
      if(!typeAlias.empty() && typeAlias != it->second) {
        LOG(ERROR)
          << "user provided invalid type "
          << typeAlias
          << " for trait: "
          << it->first
          << " Valid type is: "
          << it->second;
       CHECK(false);
      }
      validTypeAlias = it->second;
    } else {
      LOG(ERROR)
        << "user not tegistered typeclass"
        << " for trait: "
        << typeclassBaseName;
       CHECK(false);
    }

    if(reflection::ReflectionRegistry::getInstance()->
      reflectionCXXRecordRegistry.find(typeclassBaseName)
        == reflection::ReflectionRegistry::getInstance()->
          reflectionCXXRecordRegistry.end())
    {
        LOG(ERROR)
          << "ERROR: typeclassBaseName = "
          << typeclassBaseName
          << " not found!";
        CHECK(false);
        return clang_utils::SourceTransformResult{""};
    }

    const reflection::ReflectionCXXRecordRegistry*
      ReflectedBaseTypeclassRegistry =
        reflection::ReflectionRegistry::getInstance()->
          reflectionCXXRecordRegistry[typeclassBaseName].get();

    DVLOG(9) << "ReflectedBaseTypeclass->classInfoPtr_->name = "
      << ReflectedBaseTypeclassRegistry->classInfoPtr_->name;

    DVLOG(9) << "typeclassBaseName = "
      << typeclassBaseName;

    const auto fileID = SM.getMainFileID();
    const auto fileEntry = SM.getFileEntryForID(
      SM.getMainFileID());
    std::string original_full_file_path
      = fileEntry->getName();

    VLOG(9)
      << "original_full_file_path = "
      << original_full_file_path;

    {
        std::string fileTypeclassBaseName = typeclassBaseName;
        normalizeFileName(fileTypeclassBaseName);

        base::FilePath gen_hpp_name
          = outDir_.Append(targetName + "_" + fileTypeclassBaseName
            + ".typeclass_instance.generated.hpp");

        base::FilePath gen_base_typeclass_hpp_name
          = outDir_.Append(fileTypeclassBaseName
              + ".typeclass.generated.hpp");

        /*
         * Used by:
         * struct _tc_impl_t<ImplTypeclassName, BaseTypeclassName>
        >*/
        std::string& ImplTypeclassName
          = targetName;

        DCHECK(!validTypeAlias.empty());
        std::string& BaseTypeclassName
          //= ReflectedBaseTypeclassRegistry->classInfoPtr_->name;
          = validTypeAlias.empty()
            ? typeclassBaseName
            : validTypeAlias;

        reflection::ClassInfoPtr ReflectedBaseTypeclass
          = ReflectedBaseTypeclassRegistry->classInfoPtr_;

        std::string headerGuard = "";

        std::string generator_path
          = TYPECLASS_INSTANCE_TEMPLATE_CPP;
        DCHECK(!generator_path.empty())
          << TYPECLASS_INSTANCE_TEMPLATE_CPP
          << "generator_path.empty()";

        std::vector<std::string> generator_includes{
             buildIncludeDirective(
              /// \todo utf8 support
              gen_base_typeclass_hpp_name.AsUTF8Unsafe()),
             buildIncludeDirective(
              original_full_file_path),
             buildIncludeDirective(
              R"raw(type_erasure_common.hpp)raw")
          };

        // squarets will generate code from template file
        // and append it after annotated variable
        /// \note FILE_PATH defined by CMakeLists
        /// and passed to flextool via
        /// --extra-arg=-DFILE_PATH=...
        _squaretsFile(
          TYPECLASS_INSTANCE_TEMPLATE_CPP
        )
        std::string squarets_output = "";

        {
          DCHECK(squarets_output.size());
          const int writeResult = base::WriteFile(
            gen_hpp_name
            , squarets_output.c_str()
            , squarets_output.size());
          // Returns the number of bytes written, or -1 on error.
          if(writeResult == -1) {
            LOG(ERROR)
              << "Unable to write file: "
              << gen_hpp_name;
          } else {
            LOG(INFO)
              << "saved file: "
              << gen_hpp_name;
          }
        }

    }
  }
  if(processedTimes <= 0) {
    LOG(ERROR)
      << "nothing to do with "
      << sourceTransformOptions.func_with_args.func_with_args_as_string_;
    CHECK(false);
  }

  return clang_utils::SourceTransformResult{nullptr};
}

clang_utils::SourceTransformResult
  TypeclassTooling::typeclass_combo(
    const clang_utils::SourceTransformOptions& sourceTransformOptions)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(9)
    << "typeclass_combo called...";

  // EXAMPLE:
  // _typeclass_impl(
  //   typeclass_instance(target = "FireSpell", "Printable")
  // )
  // Here |typeclassBaseNames| will contain:
  // (target = "FireSpell", "Printable")
  flexlib::args typeclassBaseNames =
    sourceTransformOptions.func_with_args.parsed_func_.args_;

  clang::SourceManager &SM
    = sourceTransformOptions.rewriter.getSourceMgr();

  std::string combinedTypeclassNames;
  std::vector<std::string> typeclassNames;
  std::vector<
    std::pair<std::string, reflection::ClassInfoPtr>
    > ReflectedTypeclasses;
  std::vector<std::string> generator_includes;

  std::string targetTypeName;
  std::string typeAlias;
  for(const auto& tit : typeclassBaseNames.as_vec_) {
    if(tit.name_ == "type") {
      typeAlias = tit.value_;
      CHECK(false)
        << "custom type for typeclass combo not supported: "
        << tit.name_
        << " "
        << tit.value_;
      prepareTplArg(typeAlias);
    }

    if(tit.name_ =="name") {
      targetTypeName = tit.value_;
      prepareTplArg(targetTypeName);
    }
  }

  const size_t typeclassBaseNamesSize
    = typeclassBaseNames.as_vec_.size();

  size_t titIndex = 0;
  size_t processedTimes = 0;
  std::string fullBaseType;
  for(const auto& tit : typeclassBaseNames.as_vec_) {
      if(tit.name_ == "type") {
        continue;
      }
      if(tit.name_ == "name") {
        continue;
      }

      processedTimes++;

      std::string typeclassBaseName = tit.value_;

      VLOG(9)
        << "typeclassBaseName = "
        << typeclassBaseName;
      if(typeclassBaseName.empty()) {
          CHECK(false);
          return clang_utils::SourceTransformResult{""};
      }

      if(reflection::ReflectionRegistry::getInstance()->
        reflectionCXXRecordRegistry.find(typeclassBaseName)
          == reflection::ReflectionRegistry::getInstance()->
            reflectionCXXRecordRegistry.end())
      {
          LOG(ERROR)
            << "typeclassBaseName = "
            << typeclassBaseName
            << " not found!";
          CHECK(false);
          return clang_utils::SourceTransformResult{""};
      }

      std::string validTypeAlias = typeAlias;
      if(std::map<std::string, std::string>
          ::const_iterator it
            = traitToItsType_.find(typeclassBaseName)
         ; it != traitToItsType_.end())
      {
        if(!typeAlias.empty() && typeAlias != it->second) {
          LOG(ERROR)
            << "user provided invalid type "
            << typeAlias
            << " for trait: "
            << it->first
            << " Valid type is: "
            << it->second;
         CHECK(false);
        }
        validTypeAlias = it->second;
        fullBaseType += validTypeAlias;
        fullBaseType += ",";
      } else {
        LOG(ERROR)
          << "user not tegistered typeclass"
          << " for trait: "
          << typeclassBaseName;
         CHECK(false);
      }

      const reflection::ReflectionCXXRecordRegistry*
        ReflectedBaseTypeclassRegistry =
          reflection::ReflectionRegistry::getInstance()->
            reflectionCXXRecordRegistry[typeclassBaseName].get();

      combinedTypeclassNames
        += typeclassBaseName
        + (titIndex < (typeclassBaseNamesSize - 1) ? "_" : "");

      DCHECK(!validTypeAlias.empty());
      typeclassNames.push_back(
        validTypeAlias.empty()
        ? ReflectedBaseTypeclassRegistry->classInfoPtr_->name
        : validTypeAlias
      );

      generator_includes.push_back(
        buildIncludeDirective(
          typeclassBaseName + R"raw(.typeclass.generated.hpp)raw"));

      DVLOG(9)
        << "ReflectedBaseTypeclass->classInfoPtr_->name = "
        << ReflectedBaseTypeclassRegistry->classInfoPtr_->name;

      DVLOG(9)
        << "typeclassBaseName = "
        << typeclassBaseName;

      VLOG(9)
        << "ReflectedBaseTypeclass is record = "
        << ReflectedBaseTypeclassRegistry->classInfoPtr_->name;

      if(reflection::ReflectionRegistry::getInstance()->
        reflectionCXXRecordRegistry.find(typeclassBaseName)
          == reflection::ReflectionRegistry::getInstance()->
            reflectionCXXRecordRegistry.end())
      {
          LOG(ERROR)
            << "typeclassBaseName = "
            << typeclassBaseName
            << " not found!";
          CHECK(false);
          return clang_utils::SourceTransformResult{""};
      }

      ReflectedTypeclasses.push_back(std::make_pair(
        typeclassBaseName
        , ReflectedBaseTypeclassRegistry->classInfoPtr_));

      titIndex++;
  }
  if(processedTimes <= 0) {
    LOG(ERROR)
      << "nothing to do with "
      << sourceTransformOptions.func_with_args.func_with_args_as_string_;
    CHECK(false);
  }
  // remove last comma
  if (!fullBaseType.empty()) {
    fullBaseType.pop_back();
  }

  if(typeclassNames.empty()) {
    LOG(ERROR)
      << "typeclassNames = empty!";
    CHECK(false);
    return clang_utils::SourceTransformResult{""};
  }

  clang::SourceLocation startLoc
    = sourceTransformOptions.decl->getLocStart();
  clang::SourceLocation endLoc
    = sourceTransformOptions.decl->getLocEnd();
  clang_utils::expandLocations(
    startLoc, endLoc, sourceTransformOptions.rewriter);

  auto codeRange = clang::SourceRange{startLoc, endLoc};

  std::string OriginalTypeclassBaseCode =
    sourceTransformOptions.rewriter.getRewrittenText(codeRange);

  base::FilePath gen_hpp_name
    = outDir_.Append(
        (targetTypeName.empty()
        ? combinedTypeclassNames
        : targetTypeName)
        + ".typeclass_combo.generated.hpp");

  generator_includes.push_back(
    buildIncludeDirective(R"raw(type_erasure_common.hpp)raw"));

  {
    std::string headerGuard = "";

    std::string generator_path
      = TYPECLASS_COMBO_TEMPLATE_CPP;
    DCHECK(!generator_path.empty())
      << "generator_path.empty()"
      << TYPECLASS_COMBO_TEMPLATE_CPP;

    const auto fileID = SM.getMainFileID();
    const auto fileEntry = SM.getFileEntryForID(
      SM.getMainFileID());
    std::string full_file_path = fileEntry->getName();

    // squarets will generate code from template file
    // and append it after annotated variable
    /// \note FILE_PATH defined by CMakeLists
    /// and passed to flextool via
    /// --extra-arg=-DFILE_PATH=...
    _squaretsFile(
      TYPECLASS_COMBO_TEMPLATE_HPP
    )
    std::string squarets_output = "";

    {
      DCHECK(squarets_output.size());
      const int writeResult = base::WriteFile(
        gen_hpp_name
        , squarets_output.c_str()
        , squarets_output.size());
      // Returns the number of bytes written, or -1 on error.
      if(writeResult == -1) {
        LOG(ERROR)
          << "Unable to write file: "
          << gen_hpp_name;
      } else {
        LOG(INFO)
          << "saved file: "
          << gen_hpp_name;
      }
    }

  }

  {
    std::string headerGuard = "";

    base::FilePath gen_cpp_name
      = outDir_.Append(
          (targetTypeName.empty()
            ? combinedTypeclassNames
            : targetTypeName)
            + ".typeclass_combo.generated.cpp");

    std::map<std::string, std::any> cxtpl_params;;

    std::string generator_path
      = TYPECLASS_COMBO_TEMPLATE_HPP;
    DCHECK(!generator_path.empty())
      << "generator_path.empty()"
      << TYPECLASS_COMBO_TEMPLATE_HPP;

    generator_includes.push_back(
         buildIncludeDirective(
          /// \todo utf8 support
          gen_hpp_name.AsUTF8Unsafe()));

    const auto fileID = SM.getMainFileID();
    const auto fileEntry = SM.getFileEntryForID(
      SM.getMainFileID());
    std::string full_file_path = fileEntry->getName();

    // squarets will generate code from template file
    // and append it after annotated variable
    /// \note FILE_PATH defined by CMakeLists
    /// and passed to flextool via
    /// --extra-arg=-DFILE_PATH=...
    _squaretsFile(
      TYPECLASS_COMBO_TEMPLATE_CPP
    )
    std::string squarets_output = "";

    {
      DCHECK(squarets_output.size());
      const int writeResult = base::WriteFile(
        gen_cpp_name
        , squarets_output.c_str()
        , squarets_output.size());
      // Returns the number of bytes written, or -1 on error.
      if(writeResult == -1) {
        LOG(ERROR)
          << "Unable to write file: "
          << gen_cpp_name;
      } else {
        LOG(INFO)
          << "saved file: "
          << gen_cpp_name;
      }
    }

  }

  return clang_utils::SourceTransformResult{nullptr};
}

} // namespace plugin
