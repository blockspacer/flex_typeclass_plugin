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
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/RecordLayout.h>

#include <llvm/Support/raw_ostream.h>

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
#include <sstream>

namespace plugin {

static constexpr const char kSingleComma = ',';

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

  const clang::CXXRecordDecl* nodeRecordDecl =
    sourceTransformOptions.matchResult.Nodes.getNodeAs<
      clang::CXXRecordDecl>("bind_gen");

  clang::PrintingPolicy printingPolicy(
    sourceTransformOptions.rewriter.getLangOpts());

  const clang::LangOptions& langOptions
    = sourceTransformOptions.rewriter.getLangOpts();

  if (!nodeRecordDecl) {
    LOG(ERROR)
      << "CXXRecordDecl not found ";
    CHECK(false);
    return clang_utils::SourceTransformResult{nullptr};
  }

  /// \todo support custom namespaces
  reflection::NamespacesTree m_namespaces;

  std::string fullBaseType;

  /*
   * example input:
      struct
      _typeclass(
        "generator = InPlace"
        ", BufferSize = 64")
      MagicLongType
        : public MagicTemplatedTraits<std::string, int>
        , public ParentTemplatedTraits_1<const char *>
        , public ParentTemplatedTraits_2<const int &>
      {};
   *
   * exatracted node->bases()
   * via clang::Lexer::getSourceText(clang::CharSourceRange) are:
   *
   *  public MagicTemplatedTraits<std::string, int>
   *  , public ParentTemplatedTraits_1<const char *>
   *  , public ParentTemplatedTraits_2<const int &>
  */
  std::string baseClassesCode;

  if (nodeRecordDecl) {
    reflection::AstReflector reflector(
      sourceTransformOptions.matchResult.Context);

    std::vector<clang::CXXRecordDecl *> nodes;
    std::vector<reflection::ClassInfoPtr> structInfos;

    /// \todo replace with std::vector<MethodInfoPtr>
    reflection::ClassInfoPtr structInfo;

    DCHECK(nodeRecordDecl->bases().begin()
      != nodeRecordDecl->bases().end())
      << "(typeclass) no bases for: "
      << nodeRecordDecl->getNameAsString();
    /// \todo make recursive (support bases of bases)
    for(const clang::CXXBaseSpecifier& it
        : nodeRecordDecl->bases())
    {
      {
        clang::SourceRange varSourceRange
          = it.getSourceRange();
        clang::CharSourceRange charSourceRange(
          varSourceRange,
          true // IsTokenRange
        );
        clang::SourceLocation initStartLoc
          = charSourceRange.getBegin();
        if(varSourceRange.isValid()) {
          StringRef sourceText
            = clang::Lexer::getSourceText(
                charSourceRange
                , SM, langOptions, 0);
          DCHECK(initStartLoc.isValid());
          baseClassesCode += sourceText.str();
          baseClassesCode += kSingleComma;
        } else {
          DCHECK(false);
        }
      }
      CHECK(it.getType()->getAsCXXRecordDecl())
        << "failed getAsCXXRecordDecl for "
        << it.getTypeSourceInfo()->getType().getAsString();
      if(it.getType()->getAsCXXRecordDecl()) {
        nodes.push_back(
          it.getType()->getAsCXXRecordDecl());

        const reflection::ClassInfoPtr refled
          = reflector.ReflectClass(
              it.getType()->getAsCXXRecordDecl()
              , &m_namespaces
              , false // recursive
            );
        DCHECK(refled);
        CHECK(!refled->methods.empty())
          << "no methods in "
          << refled->name;

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
          = exatractTypeName(
              preparedFullBaseType
            );

        structInfo->compoundId.push_back(
          preparedFullBaseType);

        fullBaseType += preparedFullBaseType;
        fullBaseType += kSingleComma;
      }
    }

    // remove last comma
    if (!baseClassesCode.empty()) {
      DCHECK(baseClassesCode.back() == kSingleComma);
      baseClassesCode.pop_back();
    }

    VLOG(9)
      << "baseClassesCode: "
      << baseClassesCode
      << " of "
      << nodeRecordDecl->getNameAsString();

    // remove last comma
    if (!fullBaseType.empty()) {
      DCHECK(fullBaseType.back() == kSingleComma);
      fullBaseType.pop_back();
    }

    if(nodes.empty() || structInfos.empty()
       || !structInfo) {
      CHECK(false);
      return clang_utils::SourceTransformResult{""};
    }

    const std::string& targetTypeName
      = nodeRecordDecl->getNameAsString();

    std::string targetGenerator;
    for(const auto& tit : typeclassBaseNames.as_vec_) {
      if(tit.name_ == "generator")
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
      const clang::QualType& paramType =
        //template_type->getDefaultArgument();
        sourceTransformOptions.matchResult.Context
          ->getTypeDeclType(nodeRecordDecl);
      std::string registryTargetName =
        exatractTypeName(
          paramType.getAsString(printingPolicy)
        );

      VLOG(9) << "ReflectionRegistry... for record " <<
        registryTargetName;

      VLOG(9)
        << "registering type for trait..."
        << registryTargetName
        << " "
        << fullBaseType;

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
        DCHECK(ReflectedStructInfo);

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
        DCHECK(ReflectedStructInfo);

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

  clang::PrintingPolicy printingPolicy(
    sourceTransformOptions.rewriter.getLangOpts());

  std::string targetName;

  std::string typeclassQualType;

  std::string typeclassBaseTypeIdentifier;

  clang::QualType typeclassArgQualType;

  const clang::CXXRecordDecl *node =
    sourceTransformOptions.matchResult.Nodes.getNodeAs<
      clang::CXXRecordDecl>("bind_gen");

  const clang::LangOptions& langOptions
    = sourceTransformOptions.rewriter.getLangOpts();

  CHECK(node->getDescribedClassTemplate())
    << "node "
    << node->getNameAsString()
    << " must be template";

  /// \brief Retrieves the class template that is described by this
  /// class declaration.
  ///
  /// Every class template is represented as a ClassTemplateDecl and a
  /// CXXRecordDecl. The former contains template properties (such as
  /// the template parameter lists) while the latter contains the
  /// actual description of the template's
  /// contents. ClassTemplateDecl::getTemplatedDecl() retrieves the
  /// CXXRecordDecl that from a ClassTemplateDecl, while
  /// getDescribedClassTemplate() retrieves the ClassTemplateDecl from
  /// a CXXRecordDecl.
  if(node->getDescribedClassTemplate()) {
    clang::TemplateDecl* templateDecl =
      node->getDescribedClassTemplate();
    clang::TemplateParameterList *tp =
      templateDecl->getTemplateParameters();
    //clang::TemplateParameterList *tArgs =
    //  templateDecl->getTemplateA();
    for(clang::NamedDecl *parameter_decl: *tp) {
      CHECK(!parameter_decl->isParameterPack())
        << node->getNameAsString();

      // example: namespace::IntSummable
      std::string parameterQualType;

      // example: IntSummable (without namespace)
      std::string parameterBaseTypeIdentifier;

      clang::QualType defaultArgQualType;

      /// \brief Declaration of a template type parameter.
      ///
      /// For example, "T" in
      /// \code
      /// template<typename T> class vector;
      /// \endcode
      if (clang::TemplateTypeParmDecl* template_type
          = clang::dyn_cast<clang::TemplateTypeParmDecl>(parameter_decl))
      {
        CHECK(template_type->wasDeclaredWithTypename())
          << node->getNameAsString();
        CHECK(template_type->hasDefaultArgument())
          << node->getNameAsString();

        CHECK(parameterQualType.empty())
          << node->getNameAsString();
        defaultArgQualType =
          template_type->getDefaultArgument();
        parameterQualType
          = exatractTypeName(
              defaultArgQualType.getAsString(printingPolicy)
            );
        CHECK(!parameterQualType.empty())
          << node->getNameAsString();

        if(defaultArgQualType.getBaseTypeIdentifier()) {
          parameterBaseTypeIdentifier
            = exatractTypeName(
                  defaultArgQualType.getBaseTypeIdentifier()->getName().str()
              );
        }
      }
      /// NonTypeTemplateParmDecl - Declares a non-type template parameter,
      /// e.g., "Size" in
      /// @code
      /// template<int Size> class array { };
      /// @endcode
      else if (clang::NonTypeTemplateParmDecl* non_template_type
          = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(parameter_decl))
      {
        CHECK(parameterQualType.empty())
          << node->getNameAsString();
        CHECK(non_template_type->hasDefaultArgument())
          << node->getNameAsString();
        llvm::raw_string_ostream out(parameterQualType);
        defaultArgQualType =
          template_type->getDefaultArgument();
        non_template_type->getDefaultArgument()->printPretty(
          out
          , nullptr // clang::PrinterHelper
          , printingPolicy
          , 0 // indent
        );
        CHECK(!parameterQualType.empty())
          << node->getNameAsString();
      }
      CHECK(!parameterQualType.empty())
          << node->getNameAsString();

      DVLOG(9)
        << " template parameter decl: "
        << parameter_decl->getNameAsString()
        << " parameterQualType: "
        << parameterQualType;

      /**
       * Example input:
          template<
            IntSummable typeclass_target
            , FireSpell impl_target
          >
          struct
          _typeclass_instance()
          FireSpell_MagicItem
          {};
       **/
      if(parameter_decl->getNameAsString() == "typeclass_target") {
        CHECK(typeclassQualType.empty())
          << node->getNameAsString();
        CHECK(!parameterQualType.empty())
          << node->getNameAsString();
        typeclassQualType
          = exatractTypeName(parameterQualType);
        CHECK(!parameterBaseTypeIdentifier.empty())
          << node->getNameAsString();
        typeclassBaseTypeIdentifier
          = exatractTypeName(parameterBaseTypeIdentifier);
        CHECK(!defaultArgQualType.isNull())
          << node->getNameAsString();
        CHECK(defaultArgQualType->getAsCXXRecordDecl())
          << node->getNameAsString();
        typeclassArgQualType = defaultArgQualType;
      }
      else if(parameter_decl->getNameAsString() == "impl_target") {
        CHECK(targetName.empty())
          << node->getNameAsString();
        CHECK(!parameterQualType.empty())
          << node->getNameAsString();
        targetName
          = exatractTypeName(parameterQualType);
      } else {
        LOG(ERROR)
          << "Unknown argument "
          << parameter_decl->getNameAsString()
          << " in typeclass instance: "
          << node->getNameAsString();
        CHECK(false)
          << node->getNameAsString();
      }
    }
  }

  if (targetName.empty()) {
      LOG(ERROR)
        << "target for typeclass instance not found ";
      CHECK(false)
        << node->getNameAsString();
      return clang_utils::SourceTransformResult{nullptr};
  }

  CHECK(!targetName.empty())
    << node->getNameAsString();

  CHECK(!typeclassQualType.empty())
    << node->getNameAsString();

  {

    VLOG(9)
      << "typeclassQualType = "
      << typeclassQualType;

    VLOG(9)
      << "target = "
      << targetName;

    VLOG(9)
      << "node->getNameAsString() = "
      << node->getNameAsString();

    if(typeclassQualType.empty()) {
      CHECK(false);
      return clang_utils::SourceTransformResult{""};
    }

    DVLOG(9) << "typeclassQualType = "
      << typeclassQualType;

    const auto fileID = SM.getMainFileID();
    const auto fileEntry = SM.getFileEntryForID(
      SM.getMainFileID());
    std::string original_full_file_path
      = fileEntry->getName();

    VLOG(9)
      << "original_full_file_path = "
      << original_full_file_path;

    {
        DCHECK(!typeclassBaseTypeIdentifier.empty());
        std::string fileTypeclassBaseName
          = typeclassBaseTypeIdentifier;
        normalizeFileName(fileTypeclassBaseName);
        DCHECK(!fileTypeclassBaseName.empty());

        DCHECK(!node->getNameAsString().empty());
        base::FilePath gen_hpp_name
          = outDir_.Append(
            node->getNameAsString()
            + ".typeclass_instance.generated.hpp");

        base::FilePath gen_base_typeclass_hpp_name
          = outDir_.Append(fileTypeclassBaseName
              + ".typeclass.generated.hpp");

        /*
         * Used by:
         * struct _tc_impl_t<ImplTypeclassName, TypeclassBasesCode>
        >*/
        std::string& ImplTypeclassName
          = targetName;

        reflection::AstReflector reflector(
          sourceTransformOptions.matchResult.Context);
        DCHECK(typeclassArgQualType->getAsCXXRecordDecl());
        /// \todo support custom namespaces
        reflection::NamespacesTree m_namespaces;
        reflection::ClassInfoPtr ReflectedBaseTypeclass
          = reflector.ReflectClass(
              typeclassArgQualType->getAsCXXRecordDecl()
              , &m_namespaces
              , true // recursive
            );
        DCHECK(ReflectedBaseTypeclass);
        CHECK(typeclassArgQualType
              ->getAsCXXRecordDecl()->hasDefinition())
          << "no definition in "
          << typeclassArgQualType
              ->getAsCXXRecordDecl()->getNameAsString();
        DCHECK(typeclassArgQualType
              ->getAsCXXRecordDecl()->bases().begin()
          != typeclassArgQualType
              ->getAsCXXRecordDecl()->bases().end())
          << "(typeclass instance) no bases for: "
          << typeclassArgQualType
              ->getAsCXXRecordDecl()->getNameAsString();
        /// \todo make recursive (support bases of bases)
        for(const clang::CXXBaseSpecifier& it
            : typeclassArgQualType->getAsCXXRecordDecl()->bases())
        {
          CHECK(it.getType()->getAsCXXRecordDecl())
            << "failed getAsCXXRecordDecl for "
            << it.getTypeSourceInfo()->getType().getAsString();
          if(it.getType()->getAsCXXRecordDecl()) {
            const reflection::ClassInfoPtr refled
              = reflector.ReflectClass(
                  it.getType()->getAsCXXRecordDecl()
                  , &m_namespaces
                  , false // recursive
                );
            DCHECK(refled);
            CHECK(!refled->methods.empty())
              << "no methods in "
              << refled->name;

            DCHECK(ReflectedBaseTypeclass);
            {
              for(const auto& it : refled->members) {
                ReflectedBaseTypeclass->members.push_back(it);
              }
              for(const auto& it : refled->methods) {
                ReflectedBaseTypeclass->methods.push_back(it);
              }
              for(const auto& it : refled->innerDecls) {
                ReflectedBaseTypeclass->innerDecls.push_back(it);
              }
            }
          }
        }
        CHECK(!ReflectedBaseTypeclass->methods.empty())
          << "no methods in "
          << ReflectedBaseTypeclass->name;
        bool hasAtLeastOneValidMethod = false;
        for(const reflector::MethodInfoPtr& method
          : ReflectedBaseTypeclass->methods)
        {
          DCHECK(method);

          const size_t methodParamsSize = method->params.size();
          const bool needPrint = isTypeclassMethod(method);
          if(needPrint) {
            hasAtLeastOneValidMethod = needPrint;
          }
          // log all methods
          VLOG(9)
            << "ReflectedBaseTypeclass: "
            << ReflectedBaseTypeclass->name
            << " method: "
            << method->name;
        }
        CHECK(hasAtLeastOneValidMethod)
          << "no valid methods in "
          << ReflectedBaseTypeclass->name;
        /*
         * example input:
            struct
            _typeclass(
              "generator = InPlace"
              ", BufferSize = 64")
            MagicLongType
              : public MagicTemplatedTraits<std::string, int>
              , public ParentTemplatedTraits_1<const char *>
              , public ParentTemplatedTraits_2<const int &>
            {};
         *
         * exatracted node->bases()
         * via clang::Lexer::getSourceText(clang::CharSourceRange) are:
         *
         *  MagicTemplatedTraits<std::string, int>
         *  , ParentTemplatedTraits_1<const char *>
         *  , ParentTemplatedTraits_2<const int &>
         *
         * (without access specifiers like public/private)
        */
        std::string TypeclassBasesCode;
        {
          DCHECK(typeclassArgQualType->getAsCXXRecordDecl()->bases().begin()
            != typeclassArgQualType->getAsCXXRecordDecl()->bases().end())
            << "(ReflectedBaseTypeclass) no bases for: "
            << ReflectedBaseTypeclass->decl->getNameAsString();
          for(const clang::CXXBaseSpecifier& it
              : typeclassArgQualType->getAsCXXRecordDecl()->bases())
          {
            clang::SourceRange varSourceRange
              = it.getSourceRange();
            clang::CharSourceRange charSourceRange(
              varSourceRange,
              true // IsTokenRange
            );
            clang::SourceLocation initStartLoc
              = charSourceRange.getBegin();
            if(varSourceRange.isValid()) {
              DCHECK(initStartLoc.isValid());
              TypeclassBasesCode
                += exatractTypeName(
                     it.getType().getAsString(printingPolicy)
                   );
              TypeclassBasesCode += kSingleComma;
            } else {
              DCHECK(false);
            }
          }
          // remove last comma
          if (!TypeclassBasesCode.empty()) {
            DCHECK(TypeclassBasesCode.back() == kSingleComma);
            TypeclassBasesCode.pop_back();
          }
        }
        CHECK(!TypeclassBasesCode.empty())
          << "invalid bases for: "
          << ReflectedBaseTypeclass->decl->getNameAsString();
        DVLOG(9) <<
          "TypeclassBasesCode: "
          << TypeclassBasesCode
          << " for: "
          << ReflectedBaseTypeclass->decl->getNameAsString();

        std::string headerGuard = "";

        std::string generator_path
          = TYPECLASS_INSTANCE_TEMPLATE_CPP;
        DCHECK(!generator_path.empty())
          << TYPECLASS_INSTANCE_TEMPLATE_CPP
          << "generator_path.empty()";

        std::vector<std::string> generator_includes{
             //buildIncludeDirective(
              /// \todo utf8 support
             // gen_base_typeclass_hpp_name.AsUTF8Unsafe()),
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

  return clang_utils::SourceTransformResult{nullptr};
}

#if 0
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
        fullBaseType += kSingleComma;
      } else {
        LOG(ERROR)
          << "user not registered typeclass"
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
    DCHECK(fullBaseType.back() == kSingleComma);
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
#endif // 0

} // namespace plugin
