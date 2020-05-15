#include <flex_typeclass_plugin/Tooling.hpp> // IWYU pragma: associated

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

#include <any>
#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>

// provide template code in __VA_ARGS__
/// \note you can use \n to add newline
/// \note does not support #define, #include in __VA_ARGS__
#define $squarets(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" #__VA_ARGS__ )))

// squaretsString
/// \note may use `#include` e.t.c.
// example:
//   $squaretsString("#include <cling/Interpreter/Interpreter.h>")
#define $squaretsString(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" __VA_ARGS__)))

// example:
//   $squaretsFile("file/path/here")
/// \note FILE_PATH can be defined by CMakeLists
/// and passed to flextool via
/// --extra-arg=-DFILE_PATH=...
#define $squaretsFile(...) \
  __attribute__((annotate("{gen};{squaretsFile};CXTPL;" __VA_ARGS__)))

// uses Cling to execute arbitrary code at compile-time
// and run squarets on result returned by executed code
#define $squaretsCodeAndReplace(...) \
  /* generate definition required to use __attribute__ */ \
  __attribute__((annotate("{gen};{squaretsCodeAndReplace};CXTPL;" #__VA_ARGS__)))

namespace plugin {

namespace {

std::string typenameParamsFullDecls(
  const std::vector<reflection::MethodParamInfo>& params)
{
    std::string out;
    size_t paramIter = 0;
    const size_t methodParamsSize = params.size();
    for(const auto& param: params) {
      paramIter++;
      if(param.type->getAsTemplateParamType()) {
        out += "typename ";
        //out += param.type->getPrintedName();
        out += param.type
          ->getAsTemplateParamType()->decl->getName();
        if(paramIter != methodParamsSize) {
            out += ", ";
        } // paramIter != methodParamsSize
      }
    } // params endfor
    return out;
}

std::string paramsFullDecls(
  const std::vector<reflection::MethodParamInfo>& params)
{
    std::string out;
    size_t paramIter = 0;
    const size_t methodParamsSize = params.size();
    for(const auto& param: params) {
      out += param.fullDecl;
      /*if(param.type->getAsTemplate()
        || param.type->getAsTemplateParamType()) {
        out += " typename ";
      }*/
      paramIter++;
      if(paramIter != methodParamsSize) {
          out += ", ";
      } // paramIter != methodParamsSize
    } // params endfor
    return out;
}

std::string paramsCallDecls(
  const std::vector<reflection::MethodParamInfo>& params)
{
    std::string out;
    size_t paramIter = 0;
    const size_t methodParamsSize = params.size();
    for(const auto& param: params) {
        out += param.name;
        paramIter++;
        if(paramIter != methodParamsSize) {
            out += ", ";
        } // paramIter != methodParamsSize
    } // params endfor
    return out;
}

std::string templateParamsFullDecls(
  const std::vector<reflection::TemplateParamInfo>& params)
{
    std::string out;
    size_t paramIter = 0;
    const size_t methodParamsSize = params.size();
    for(const auto& param: params) {
        out += param.tplDeclName;
        paramIter++;
        if(paramIter != methodParamsSize) {
            out += ", ";
        } // paramIter != methodParamsSize
    } // params endfor
    return out;
}

std::string typeclassModelName(const std::string& arg) {
  return "model_for_" + arg;
}

std::string typeclassModelsDecls(
  const std::vector<std::string>& params)
{
    /*const std::string comboType =
      typeclassComboDecls(params);*/
    std::string out;
    for(const auto& param: params) {
      out += "\n"
             "std::shared_ptr<_tc_model_t<";
      out += param;
      out += ">> ";
      out += typeclassModelName(param);
      out += ";"
             "\n";
    } // params endfor
    return out;
}

std::string typeclassComboDecls(const std::vector<std::string>& params)
{
    std::string out;
    size_t paramIter = 0;
    const size_t methodParamsSize = params.size();
    for(const auto& param: params) {
        out += param;
        paramIter++;
        if(paramIter != methodParamsSize) {
            out += ", ";
        } // paramIter != methodParamsSize
    } // params endfor
    return out;
}

static std::string startHeaderGuard(const std::string& guardName) {
    std::string out;
    if(!guardName.empty()) {
        out += "#ifndef ";
        out += guardName;
        out += "\n";
        out += "#define ";
        out += guardName;
        out += "\n";
    } else {
        out += "#pragma once\n";
    } // !guardName.empty()
    return out;
}

std::string endHeaderGuard(const std::string& guardName) {
    std::string out;
    if(!guardName.empty()) {
        out += "#endif // ";
        out += guardName;
        out += "\n";
    }
    return out;
}

// we want to generate file names based on parsed C++ types.
// cause file names can not contain spaces ( )
// and punctuations (,.:') we want to
// replace special characters in filename to '_'
// BEFORE:
//   $typeclass_impl(
//     typeclass_instance(
//       target = "FireSpell",
//       "MagicTemplated<std::string, int>,"
//       "ParentTemplated_1<const char *>,"
//       "ParentTemplated_2<const int &>")
//   )
// AFTER:
//   FireSpell_MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass_instance.generated.hpp
static void normalizeFileName(std::string &in)
{
  std::replace_if(in.begin(), in.end(), ::ispunct, '_');
  std::replace_if(in.begin(), in.end(), ::isspace, '_');
}

static bool startsWith(const std::string_view& in,
    const std::string& prefix) {
  return !in.compare(0, prefix.size(), prefix);
}

static std::string_view removePrefix(const std::string_view& from,
    const std::string& prefix) {
  return from.substr( prefix.size(), from.size() - prefix.size());
}

// exatracts `SomeType` out of:
// struct SomeType{};
// OR
// class SomeType{};
// Note that `class` in clang LibTooling is `record`
static void prepareTypeName(std::string &in)
{
  {
    const std::string prefix = "struct ";
    if(startsWith(in, prefix)) {
      in = removePrefix(in, prefix);
    }
  }
  {
    const std::string prefix = "record ";
    if(startsWith(in, prefix)) {
      in = removePrefix(in, prefix);
    }
  }
}

// EXAMPLE:
// $typeclass_impl(
//   typeclass_instance(target = "FireSpell", "Printable")
// )
// Note quotes around "FireSpell":
// target = "FireSpell"
// we want to parse "FireSpell" without quotes
static void prepareTplArg(std::string &in)
{
  // remove quotes
  in.erase(
    std::remove( in.begin(), in.end(), '\"' ),
    in.end());
}

static void writeToFile(const std::string& str, const std::string& file_path) {
  fs::create_directories(fs::path(file_path).parent_path());

  std::ofstream ofs(file_path);
  if(!ofs) {
    // TODO: better error reporting
    printf("ERROR: can`t write to file %s\n", file_path.c_str());
    return;
  }
  ofs << str;
  ofs.close();
  if(!ofs)    //bad() function will check for badbit
  {
    printf("ERROR: writing to file failed %s\n", file_path.c_str());
    return;
  }
}

static std::string wrapLocalInclude(const std::string& inStr) {
    std::string result = R"raw(#include ")raw";
    result += inStr;
    result += R"raw(")raw";
    return result;
}

} // namespace

Tooling::Tooling(
  const ::plugin::ToolPlugin::Events::RegisterAnnotationMethods& event
#if defined(CLING_IS_ON)
  , ::cling_utils::ClingInterpreter* clingInterpreter
#endif // CLING_IS_ON
) : clingInterpreter_(clingInterpreter)
{
  DCHECK(clingInterpreter_);

  DETACH_FROM_SEQUENCE(sequence_checker_);

  DCHECK(event.sourceTransformPipeline);
  ::clang_utils::SourceTransformPipeline& sourceTransformPipeline
    = *event.sourceTransformPipeline;

  sourceTransformRules_
    = &sourceTransformPipeline.sourceTransformRules;
}

Tooling::~Tooling()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

clang_utils::SourceTransformResult
  Tooling::typeclass(
    const clang_utils::SourceTransformOptions& sourceTransformOptions)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(9)
    << "typeclass called...";

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
    return clang_utils::SourceTransformResult{nullptr};
  }

  reflection::NamespacesTree m_namespaces; // TODO

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
      //node = it.getType()->getAsCXXRecordDecl();//<clang::RecordDecl>();

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
        prepareTypeName(
          preparedFullBaseType);

        structInfo->compoundId.push_back(
          preparedFullBaseType);

        fullBaseType += preparedFullBaseType;
        fullBaseType += ",";
        //break;
      }
    }

    // remove last comma
    if (!fullBaseType.empty()) {
      fullBaseType.pop_back();
    }

    if(nodes.empty() || structInfos.empty()
       || !structInfo) {
      return clang_utils::SourceTransformResult{""};
    }

    {
      DLOG(INFO) << "ReflectionRegistry... for record " <<
        fullBaseType;

      reflection::ReflectionRegistry::getInstance()->
        reflectionCXXRecordRegistry[fullBaseType]
          = std::make_unique<
              reflection::ReflectionCXXRecordRegistry>(
                fullBaseType, /*node,*/ structInfo);

      DLOG(INFO) << "templateParams.size"
        << structInfo->templateParams.size();
      DLOG(INFO) << "genericParts.size"
        << structInfo->genericParts.size();

      std::string fileTargetName = fullBaseType;
      normalizeFileName(fileTargetName);

      fs::path gen_hpp_name = fs::absolute(
          fs::path("generated")
          ///\todo
          ///ctp::Options::res_path
        / (fileTargetName + ".typeclass.generated.hpp"));

      fs::path gen_cpp_name = fs::absolute(
          fs::path("generated")
          ///\todo
          ///ctp::Options::res_path
        / (fileTargetName + ".typeclass.generated.cpp"));

      {
        std::string headerGuard = "";

        std::string generator_path = TYPECLASS_TEMPLATE_HPP;

        const auto fileID = SM.getMainFileID();
        const auto fileEntry = SM.getFileEntryForID(
          SM.getMainFileID());
        std::string full_file_path = fileEntry->getName();

        std::vector<std::string> generator_includes{
             wrapLocalInclude(
              full_file_path),
             wrapLocalInclude(
              R"raw(type_erasure_common.hpp)raw")
          };

        reflection::ClassInfoPtr ReflectedStructInfo
          = structInfo;

        // squarets will generate code from template file
        // and appende it after annotated variable
        /// \note FILE_PATH defined by CMakeLists
        /// and passed to flextool via
        /// --extra-arg=-DFILE_PATH=...
        $squaretsFile(
          TYPECLASS_TEMPLATE_HPP
        )
        std::string squarets_output = "";

        writeToFile(squarets_output, gen_hpp_name);

        LOG(INFO)
          << "saved file: "
          << gen_hpp_name;
      }

      {
        std::string headerGuard = "";

        std::string generator_path = TYPECLASS_TEMPLATE_CPP;

        reflection::ClassInfoPtr ReflectedStructInfo
          = structInfo;

        const auto fileID = SM.getMainFileID();
        const auto fileEntry = SM.getFileEntryForID(
          SM.getMainFileID());
        std::string full_file_path = fileEntry->getName();

        std::vector<std::string> generator_includes{
             wrapLocalInclude(
              gen_hpp_name),
             wrapLocalInclude(
              R"raw(type_erasure_common.hpp)raw")
          };

        // squarets will generate code from template file
        // and appende it after annotated variable
        /// \note FILE_PATH defined by CMakeLists
        /// and passed to flextool via
        /// --extra-arg=-DFILE_PATH=...
        $squaretsFile(
          TYPECLASS_TEMPLATE_CPP
        )
        std::string squarets_output = "";

        writeToFile(squarets_output, gen_cpp_name);

        LOG(INFO)
          << "saved file: "
          << gen_cpp_name;
      }

#if 0
      {
        const auto fileID = SM.getMainFileID();
        const auto fileEntry = SM.getFileEntryForID(
          SM.getMainFileID());
        std::string full_file_path = fileEntry->getName();
        DLOG(INFO) << "full_file_path is "
          << full_file_path;

        std::map<std::string, std::any> cxtpl_params;

        cxtpl_params.emplace("ReflectedStructInfo",
                       std::make_any<
                        reflection::ClassInfoPtr>(structInfo));
        //DLOG(INFO) << "methods count: " << structInfo->methods.size();

        cxtpl_params.emplace("fullBaseType",
                       std::make_any<std::string>(
                        fullBaseType));

        cxtpl_params.emplace("generator_path",
                       std::make_any<std::string>(
                        "typeclass_gen_hpp.cxtpl"));

        cxtpl_params.emplace("generator_includes",
                             std::make_any<
                               std::vector<std::string>>(
                                 std::vector<std::string>{
                                     wrapLocalInclude(
                                      full_file_path),
                                     wrapLocalInclude(
                                      R"raw(type_erasure_common.hpp)raw")
                                 })
                             );

        std::string cxtpl_output;

/// \note generated file
#include "flex_typeclass_plugin/generated/typeclass_gen_hpp.cxtpl.cpp"

        writeToFile(cxtpl_output, gen_hpp_name);
      }

      {
        std::map<std::string, std::any> cxtpl_params;

        cxtpl_params.emplace("ReflectedStructInfo",
                       std::make_any<
                        reflection::ClassInfoPtr>(structInfo));
        //DLOG(INFO) << "methods count: " << structInfo->methods.size();

        cxtpl_params.emplace("fullBaseType",
                       std::make_any<
                        std::string>(fullBaseType));

        cxtpl_params.emplace("generator_path",
                       std::make_any<
                        std::string>("typeclass_gen_cpp.cxtpl"));
        cxtpl_params.emplace("generator_includes",
                             std::make_any<
                               std::vector<std::string>>(
                                 std::vector<std::string>{
                                     wrapLocalInclude(
                                      gen_hpp_name),
                                     wrapLocalInclude(
                                      R"raw(type_erasure_common.hpp)raw")
                                 })
                             );

        std::string cxtpl_output;

/// \note generated file
#include "flex_typeclass_plugin/generated/typeclass_gen_cpp.cxtpl.cpp"

        writeToFile(cxtpl_output, gen_cpp_name);
      }
#endif // 0

    }
  }

  return clang_utils::SourceTransformResult{nullptr};
}

clang_utils::SourceTransformResult
  Tooling::typeclass_instance(
    const clang_utils::SourceTransformOptions& sourceTransformOptions)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(9)
    << "typeclass_instance called...";

  // EXAMPLE:
  // $typeclass_impl(
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
    if(tit.name_ =="target") {
      targetName = tit.value_;
      prepareTplArg(targetName);
    }
  }

  if (targetName.empty()) {
      LOG(ERROR)
        << "target for typeclass instance not found ";
      return clang_utils::SourceTransformResult{nullptr};
  }

  for(const auto& tit : typeclassBaseNames.as_vec_) {
    if(tit.name_ =="target") {
      continue;
    }

    std::string typeclassBaseName = tit.value_;
    prepareTplArg(typeclassBaseName);

    DLOG(INFO)
      << "typeclassBaseName = "
      << typeclassBaseName;

    DLOG(INFO)
      << "target = "
      << targetName;

    if(typeclassBaseName.empty()) {
        return clang_utils::SourceTransformResult{""};
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
        return clang_utils::SourceTransformResult{""};
    }

    const reflection::ReflectionCXXRecordRegistry*
      ReflectedBaseTypeclassRegistry =
        reflection::ReflectionRegistry::getInstance()->
          reflectionCXXRecordRegistry[typeclassBaseName].get();

    /*DLOG(INFO) << "ReflectedBaseTypeclass->classInfoPtr_->name = "
      << ReflectedBaseTypeclass->classInfoPtr_->name;
    DLOG(INFO) << "typeclassBaseName = "
      << typeclassBaseName;*/

    const auto fileID = SM.getMainFileID();
    const auto fileEntry = SM.getFileEntryForID(
      SM.getMainFileID());
    std::string original_full_file_path
      = fileEntry->getName();

    DLOG(INFO)
      << "original_full_file_path = "
      << original_full_file_path;

    {
        std::string fileTypeclassBaseName = typeclassBaseName;
        normalizeFileName(fileTypeclassBaseName);

        fs::path gen_hpp_name = fs::absolute(
          fs::path("generated")
          ///\todo
          ///ctp::Options::res_path
          / (targetName + "_" + fileTypeclassBaseName
            + ".typeclass_instance.generated.hpp"));

        fs::path gen_base_typeclass_hpp_name =
          fs::absolute(
            fs::path("generated")
            ///\todo
            ///ctp::Options::res_path
            / (fileTypeclassBaseName
              + ".typeclass.generated.hpp"));

        std::string& ImplTypeclassName
          = targetName;

        std::string& BaseTypeclassName
          = typeclassBaseName;

        reflection::ClassInfoPtr ReflectedBaseTypeclass
          = ReflectedBaseTypeclassRegistry->classInfoPtr_;

        std::string headerGuard = "";

        std::string generator_path
          = TYPECLASS_INSTANCE_TEMPLATE_CPP;
          // "typeclass_instance_gen_hpp.cxtpl";

        std::vector<std::string> generator_includes{
             wrapLocalInclude(
              gen_base_typeclass_hpp_name),
             wrapLocalInclude(
              original_full_file_path),
             wrapLocalInclude(
              R"raw(type_erasure_common.hpp)raw")
          };

        // squarets will generate code from template file
        // and appende it after annotated variable
        /// \note FILE_PATH defined by CMakeLists
        /// and passed to flextool via
        /// --extra-arg=-DFILE_PATH=...
        $squaretsFile(
          TYPECLASS_INSTANCE_TEMPLATE_CPP
        )
        std::string squarets_output = "";

        writeToFile(squarets_output, gen_hpp_name);

        LOG(INFO)
          << "saved file: "
          << gen_hpp_name;
#if 0
        std::map<std::string, std::any> cxtpl_params;

        cxtpl_params.emplace("ImplTypeclassName",
                             std::make_any<std::string>(
                              targetName));

        cxtpl_params.emplace("BaseTypeclassName",
                             std::make_any<std::string>(
                              typeclassBaseName));

        cxtpl_params.emplace("ReflectedBaseTypeclass",
                             std::make_any<reflection::ClassInfoPtr>(
                              ReflectedBaseTypeclass->classInfoPtr_));

        cxtpl_params.emplace("generator_path",
                             std::make_any<std::string>(
                             "typeclass_instance_gen_hpp.cxtpl"));
        cxtpl_params.emplace("generator_includes",
                             std::make_any<std::vector<std::string>>(
                                 std::vector<std::string>{
                                     /// \TODO
                                     R"raw(#include "type_erasure_common.hpp")raw",
                                     wrapLocalInclude(
                                      gen_base_typeclass_hpp_name),
                                     wrapLocalInclude(
                                      original_full_file_path)
                                 })
                             );

        std::string cxtpl_output;

/// \note generated file
#include "flex_typeclass_plugin/generated/typeclass_instance_gen_hpp.cxtpl.cpp"

        writeToFile(cxtpl_output, gen_hpp_name);
#endif // 0
    }
  }

  return clang_utils::SourceTransformResult{nullptr};
}

clang_utils::SourceTransformResult
  Tooling::typeclass_combo(
    const clang_utils::SourceTransformOptions& sourceTransformOptions)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(9)
    << "typeclass_combo called...";

  // EXAMPLE:
  // $typeclass_impl(
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
  std::vector<reflection::ClassInfoPtr> ReflectedTypeclasses;
  std::vector<std::string> generator_includes;

  const size_t typeclassBaseNamesSize
    = typeclassBaseNames.as_vec_.size();

  size_t titIndex = 0;
  for(const auto& tit : typeclassBaseNames.as_vec_) {
      const std::string typeclassBaseName = tit.value_;
      DLOG(INFO) << "typeclassBaseName = " << typeclassBaseName;
      if(typeclassBaseName.empty()) {
          return clang_utils::SourceTransformResult{""};
      }

      if(reflection::ReflectionRegistry::getInstance()->
        reflectionCXXRecordRegistry.find(typeclassBaseName)
          == reflection::ReflectionRegistry::getInstance()->
            reflectionCXXRecordRegistry.end())
      {
          LOG(ERROR) << "typeclassBaseName = "
            << typeclassBaseName << " not found!";
          return clang_utils::SourceTransformResult{""};
      }

      const reflection::ReflectionCXXRecordRegistry*
        ReflectedBaseTypeclassRegistry =
          reflection::ReflectionRegistry::getInstance()->
            reflectionCXXRecordRegistry[typeclassBaseName].get();

      combinedTypeclassNames += typeclassBaseName
        + (titIndex < (typeclassBaseNamesSize - 1) ? "_" : "");

      typeclassNames.push_back(typeclassBaseName);
      generator_includes.push_back(
        wrapLocalInclude(
          typeclassBaseName + R"raw(.typeclass.generated.hpp)raw"));

      /*DLOG(INFO) << "ReflectedBaseTypeclass->classInfoPtr_->name = "
        << ReflectedBaseTypeclass->classInfoPtr_->name;
      DLOG(INFO) << "typeclassBaseName = "
        << typeclassBaseName;*/

      DLOG(INFO) << "ReflectedBaseTypeclass is record = "
        << ReflectedBaseTypeclassRegistry->classInfoPtr_->name;

      if(reflection::ReflectionRegistry::getInstance()->
        reflectionCXXRecordRegistry.find(typeclassBaseName)
          == reflection::ReflectionRegistry::getInstance()->
            reflectionCXXRecordRegistry.end())
      {
          LOG(ERROR) << "typeclassBaseName = "
            << typeclassBaseName << " not found!";
          return clang_utils::SourceTransformResult{""};
      }

      ReflectedTypeclasses.push_back(
        ReflectedBaseTypeclassRegistry->classInfoPtr_);

      titIndex++;
  }

  if(typeclassNames.empty()) {
    LOG(ERROR) << "typeclassNames = empty!";
    return clang_utils::SourceTransformResult{""};
  }

  // see https://github.com/asutton/clang/blob/master/lib/AST/DeclPrinter.cpp#L502

  clang::SourceLocation startLoc
    = sourceTransformOptions.decl->getLocStart();
  clang::SourceLocation endLoc
    = sourceTransformOptions.decl->getLocEnd();
  clang_utils::expandLocations(
    startLoc, endLoc, sourceTransformOptions.rewriter);

  auto codeRange = clang::SourceRange{startLoc, endLoc};

  std::string OriginalTypeclassBaseCode =
    sourceTransformOptions.rewriter.getRewrittenText(codeRange);

  // removes $apply(typeclass, e.t.c.)
  /*std::string CleanOriginalTypeclassBaseCode
    = std::regex_replace(OriginalTypeclassBaseCode,
        std::regex("\\$apply([^(]*)\\([^)]*\\)(.*)"), "$1$2");*/

  fs::path gen_hpp_name = fs::absolute(
    fs::path("generated")
    ///\todo
    ///ctp::Options::res_path
    / (combinedTypeclassNames + ".typeclass_combo.generated.hpp"));

  generator_includes.push_back(
    wrapLocalInclude(R"raw(type_erasure_common.hpp)raw"));

  {
    std::string headerGuard = "";

    std::string generator_path = TYPECLASS_COMBO_TEMPLATE_CPP;

    const auto fileID = SM.getMainFileID();
    const auto fileEntry = SM.getFileEntryForID(
      SM.getMainFileID());
    std::string full_file_path = fileEntry->getName();

    // squarets will generate code from template file
    // and append it after annotated variable
    /// \note FILE_PATH defined by CMakeLists
    /// and passed to flextool via
    /// --extra-arg=-DFILE_PATH=...
    $squaretsFile(
      TYPECLASS_COMBO_TEMPLATE_HPP
    )
    std::string squarets_output = "";

    writeToFile(squarets_output, gen_hpp_name);

    LOG(INFO)
      << "saved file: "
      << gen_hpp_name;
  }

#if 0
  {
    std::map<std::string, std::any> cxtpl_params;

    cxtpl_params.emplace("typeclassNames",
                   std::make_any<std::vector<std::string>>(
                    typeclassNames));

    cxtpl_params.emplace("ReflectedTypeclasses",
                   std::make_any<std::vector<reflection::ClassInfoPtr>>(
                    ReflectedTypeclasses));

    cxtpl_params.emplace("generator_path",
                   std::make_any<std::string>(
                    "typeclass_combo_gen_hpp.cxtpl"));

    generator_includes.push_back(
      wrapLocalInclude(R"raw(type_erasure_common.hpp)raw"));

    cxtpl_params.emplace("generator_includes",
                         std::make_any<std::vector<std::string>>(
                             std::move(generator_includes))
                         );

    std::string cxtpl_output;

/// \note generated file
#include "flex_typeclass_plugin/generated/typeclass_combo_gen_hpp.cxtpl.cpp"

    writeToFile(cxtpl_output, gen_hpp_name);
  }

  {
    fs::path gen_cpp_name = fs::absolute(
      fs::path("generated")
      ///\todo
      ///ctp::Options::res_path
      / (combinedTypeclassNames + ".typeclass_combo.generated.cpp"));

    std::map<std::string, std::any> cxtpl_params;

    cxtpl_params.emplace("typeclassNames",
                   std::make_any<std::vector<std::string>>(
                    typeclassNames));

    cxtpl_params.emplace("ReflectedTypeclasses",
                   std::make_any<std::vector<reflection::ClassInfoPtr>>(
                    ReflectedTypeclasses));

    cxtpl_params.emplace("generator_path",
                   std::make_any<std::string>(
                    "typeclass_combo_gen_cpp.cxtpl"));

    generator_includes.push_back(
      wrapLocalInclude(R"raw(type_erasure_common.hpp)raw"));

    generator_includes.push_back(
      wrapLocalInclude(gen_hpp_name));

    cxtpl_params.emplace("generator_includes",
                         std::make_any<std::vector<std::string>>(
                             std::move(generator_includes))
                         );

    std::string cxtpl_output;

/// \note generated file
#include "flex_typeclass_plugin/generated/typeclass_combo_gen_cpp.cxtpl.cpp"

    writeToFile(cxtpl_output, gen_cpp_name);
  }
#endif // 0

  {
    std::string headerGuard = "";

    fs::path gen_cpp_name = fs::absolute(
      fs::path("generated")
      ///\todo
      ///ctp::Options::res_path
      / (combinedTypeclassNames + ".typeclass_combo.generated.cpp"));

    std::map<std::string, std::any> cxtpl_params;;

    std::string generator_path = TYPECLASS_COMBO_TEMPLATE_HPP;

    generator_includes.push_back(
         wrapLocalInclude(
          gen_hpp_name));

    const auto fileID = SM.getMainFileID();
    const auto fileEntry = SM.getFileEntryForID(
      SM.getMainFileID());
    std::string full_file_path = fileEntry->getName();

    // squarets will generate code from template file
    // and append it after annotated variable
    /// \note FILE_PATH defined by CMakeLists
    /// and passed to flextool via
    /// --extra-arg=-DFILE_PATH=...
    $squaretsFile(
      TYPECLASS_COMBO_TEMPLATE_CPP
    )
    std::string squarets_output = "";

    writeToFile(squarets_output, gen_cpp_name);

    LOG(INFO)
      << "saved file: "
      << gen_cpp_name;
  }

  return clang_utils::SourceTransformResult{nullptr};
}

} // namespace plugin
