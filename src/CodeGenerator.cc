#include "flex_typeclass_plugin/CodeGenerator.hpp" // IWYU pragma: associated

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

#include <any>
#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>

namespace reflection {

ReflectionRegistry *ReflectionRegistry::instance;

ReflectionRegistry *ReflectionRegistry::getInstance() {
    if (!instance)
        instance = new ReflectionRegistry;
    return instance;
}

ReflectionCXXRecordRegistry::ReflectionCXXRecordRegistry(const reflectionID &id/*, const CXXRecordDecl *node*/, ClassInfoPtr classInfoPtr)
    : id_(id)/*, node_(node)*/, classInfoPtr_(classInfoPtr) {
}

} // namespace reflection

namespace plugin {

// extern
const char kSeparatorWhitespace[] = " ";

// extern
const char kSeparatorCommaAndWhitespace[] = ", ";

// extern
const char kStructPrefix[] = "struct ";

// extern
const char kRecordPrefix[] = "record ";

// extern
const char kSettingsPluginName[] = "flex_typeclass_plugin";

// extern
const char kOutDirOption[] = "out_dir";

void forEachDeclaredMethod(
  const std::vector<reflection::MethodInfoPtr>& methods
  , const base::RepeatingCallback<
      void(
        const reflection::MethodInfoPtr&
        , size_t)
    >& func
){
  size_t index = 0;
  for(const reflection::MethodInfoPtr& method : methods) {
    func.Run(method, index);
    index++;
  }
}

bool isTypeclassMethod(
  const reflection::MethodInfoPtr& methodInfo)
{
  // only normal member functions should have
  // wrappers generated for them
  return !methodInfo->isImplicit
      && !methodInfo->isOperator
      && !methodInfo->isCtor
      && !methodInfo->isDtor;
}

std::string expandTemplateTypes(
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
        out += kSeparatorCommaAndWhitespace;
      } // paramIter != methodParamsSize
    }
  } // params endfor
  return out;
}

std::string expandTemplateNames(
  const std::vector<reflection::TemplateParamInfo>& params)
{
  std::string out;
  size_t paramIter = 0;
  const size_t methodParamsSize = params.size();
  for(const auto& param: params) {
    out += param.tplDeclName;
    paramIter++;
    if(paramIter != methodParamsSize) {
      out += kSeparatorCommaAndWhitespace;
    } // paramIter != methodParamsSize
  } // params endfor
  return out;
}

std::string expandMethodParameterDeclarations(
  const std::vector<reflection::MethodParamInfo>& params)
{
  std::string out;
  size_t paramIter = 0;
  const size_t methodParamsSize = params.size();
  for(const auto& param: params) {
    out += param.fullDecl;
    paramIter++;
    if(paramIter != methodParamsSize) {
      out += kSeparatorCommaAndWhitespace;
    } // paramIter != methodParamsSize
  } // params endfor
  return out;
}

std::string expandMethodParameterNames(
  const std::vector<reflection::MethodParamInfo>& params)
{
  std::string out;
  size_t paramIter = 0;
  const size_t methodParamsSize = params.size();
  for(const auto& param: params) {
    out += param.name;
    paramIter++;
    if(paramIter != methodParamsSize) {
      out += kSeparatorCommaAndWhitespace;
    } // paramIter != methodParamsSize
  } // params endfor
  return out;
}

std::string startHeaderGuard(
  const std::string& guardName)
{
  if(!guardName.empty()) {
    // squarets will generate code from string
    // and append it after annotated variable
    _squaretsString(
R"raw(
#if !defined([[+ guardName +]])
#define [[+ guardName +]]
)raw"
    )
    std::string squarets_output = "";
    return squarets_output;
  } else {
    return "#pragma once\n";
  } // !guardName.empty()
  return "";
}

std::string endHeaderGuard(const std::string& guardName)
{
  if(!guardName.empty()) {
    // squarets will generate code from string
    // and append it after annotated variable
    _squaretsString(
R"raw(
#endif // [[+ guardName +]]

)raw"
    )
    std::string squarets_output = "";
    return squarets_output;
  }
  return "";
}

void normalizeFileName(std::string &in)
{
  std::replace_if(in.begin(), in.end(), ::ispunct, '_');
  std::replace_if(in.begin(), in.end(), ::isspace, '_');
}

std::string printMethodForwarding(
  const reflection::MethodInfoPtr& methodInfo
  , const std::string& separator
  // what method printer is allowed to print
  // |options| is a bitmask of |MethodPrinter::Options|
  , int options
){
  DCHECK(methodInfo);

  std::string result;

  const bool allowExplicit
    = (options & MethodPrinter::Forwarding::Options::EXPLICIT);

  const bool allowVirtual
    = (options & MethodPrinter::Forwarding::Options::VIRTUAL);

  const bool allowConstexpr
    = (options & MethodPrinter::Forwarding::Options::CONSTEXPR);

  const bool allowStatic
    = (options & MethodPrinter::Forwarding::Options::STATIC);

  const bool allowReturnType
    = (options & MethodPrinter::Forwarding::Options::RETURN_TYPE);

  if(allowExplicit
     && methodInfo->isExplicitCtor)
  {
    result += "explicit";
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->isExplicitCtor
          ? "method is explicit, "
          "but explicit is not printed "
          "due to provided options"
          : "method is not explicit");
  }

  if(allowVirtual
     && methodInfo->isVirtual)
  {
    result += "virtual";
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->isVirtual
          ? "method is virtual, "
          "but virtual is not printed "
          "due to provided options"
          : "method is not virtual");
  }

  if(allowConstexpr
     && methodInfo->isConstexpr)
  {
    result += "constexpr";
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->isConstexpr
          ? "method is constexpr, "
          "but constexpr is not printed "
          "due to provided options"
          : "method is not constexpr");
  }

  if(allowStatic
     && methodInfo->isStatic)
  {
    result += "static";
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->isStatic
          ? "method is static, "
          "but static is not printed "
          "due to provided options"
          : "method is not static");
  }

  DCHECK(allowReturnType ? !methodInfo->isCtor : true)
    << "constructor can not have return type, method: "
    << methodInfo->name;

  DCHECK(allowReturnType ? !methodInfo->isDtor : true)
    << "destructor can not have return type, method: "
    << methodInfo->name;

  if(allowReturnType
     && methodInfo->returnType)
  {
    DCHECK(!methodInfo->returnType->getPrintedName().empty());
    result += methodInfo->returnType->getPrintedName();
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->returnType
          ? "method has return type, "
          "but return type is not printed "
          "due to provided options"
          : "method without return type");
  }

  DCHECK(!result.empty())
    << "printMethodForwarding failed for method: "
    << methodInfo->name;
  return result;
}

std::string printMethodTrailing(
  const reflection::MethodInfoPtr& methodInfo
  , const std::string& separator
  // what method printer is allowed to print
  // |options| is a bitmask of |MethodPrinter::Trailing::Options|
  , int options
){
  DCHECK(methodInfo);

  std::string result;

  const bool allowConst
    = (options & MethodPrinter::Trailing::Options::CONST);

  const bool allowNoexcept
    = (options & MethodPrinter::Trailing::Options::NOEXCEPT);

  const bool allowPure
    = (options & MethodPrinter::Trailing::Options::PURE);

  const bool allowDeleted
    = (options & MethodPrinter::Trailing::Options::DELETED);

  const bool allowDefault
    = (options & MethodPrinter::Trailing::Options::DEFAULT);

  const bool allowBody
    = (options & MethodPrinter::Trailing::Options::BODY);

  if(allowConst
     && methodInfo->isConst)
  {
    result += "const";
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->isConst
          ? "method is const, "
          "but const is not printed "
          "due to provided options"
          : "method is not const");
  }

  if(allowNoexcept
     && methodInfo->isNoExcept)
  {
    result += "noexcept";
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->isNoExcept
          ? "method is noexcept, "
          "but noexcept is not printed "
          "due to provided options"
          : "method is not noexcept");
  }

  if(allowPure
     && methodInfo->isPure)
  {
    result += "= 0";
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->isPure
          ? "method is pure (= 0), "
          "but (= 0) is not printed "
          "due to provided options"
          : "method is not pure (= 0)");
  }

  if(allowDeleted
     && methodInfo->isDeleted)
  {
    result += "= delete";
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->isDeleted
          ? "method is deleted (= delete), "
          "but (= delete) is not printed "
          "due to provided options"
          : "method is not deleted (= delete)");
  }

  if(allowDefault
     && methodInfo->isDefault)
  {
    result += "= default";
    result += separator;
  } else {
    DVLOG(20)
      << (methodInfo->isDefault
          ? "method is default (= default), "
          "but (= default) is not printed "
          "due to provided options"
          : "method is not default (= default)");
  }

  const bool canHaveBody =
     methodInfo->isDefined
     && methodInfo->isClassScopeInlined;

  if(allowBody
     && canHaveBody)
  {
    DCHECK(!methodInfo->body.empty());
    result += methodInfo->body;
    DVLOG(20)
      << "created body for method"
      << methodInfo->name;
  }
  else if(allowBody)
  {
    // no body example: methodType methodName(methodArgs);
    result += ";";
    DVLOG(20)
      << "created empty body for method"
      << methodInfo->name;
  } else {
    DVLOG(20)
      << (canHaveBody
          ? "method can have body, "
          "but body is not printed "
          "due to provided options"
          : "method can't have body");
  }

  DCHECK(!result.empty())
    << "printMethodTrailing failed for method: "
    << methodInfo->name;
  return result;
}

std::string joinWithSeparator(
  const std::vector<std::string>& input
  , const std::string& separator
  , StrJoin join_logic
){
  DCHECK(!input.empty());
  DCHECK(!separator.empty());

  std::string result;

  for(const std::string& param: input) {
    DCHECK(!param.empty());
    result += param;
    result += separator;
  }

  switch (join_logic) {
    // joins vector<string1, string2> with separator ", "
    // into "string1, string2, " (note ending ", ")
    case StrJoin::KEEP_LAST_SEPARATOR: {
      // skip
      break;
    }
    // joins vector<string1, string2> with separator ", "
    // into "string1, string2"
    case StrJoin::STRIP_LAST_SEPARATOR: {
      if(!result.empty() && !separator.empty()) {
        const std::string::size_type trim_end_pos
          = result.length() - separator.length();
        DCHECK(trim_end_pos > 0);
        result = result.substr(0, trim_end_pos);
      }
      break;
    }

    case StrJoin::TOTAL:
    default: {
      NOTREACHED();
      return "";
    }
  }

  DCHECK(!result.empty())
    << "joinWithSeparator failed";
  return result;
}

std::string exatractTypeName(
  const std::string& input)
{
  {
    DCHECK(base::size(kStructPrefix));
    if(base::StartsWith(input, kStructPrefix
         , base::CompareCase::INSENSITIVE_ASCII))
    {
      return input.substr(base::size(kStructPrefix) - 1
                     , std::string::npos);
    }
  }

  {
    DCHECK(base::size(kRecordPrefix));
    const std::string prefix = "record ";
    if(base::StartsWith(input, kRecordPrefix
         , base::CompareCase::INSENSITIVE_ASCII))
    {
      return input.substr(base::size(kRecordPrefix) - 1
                     , std::string::npos);
    }
  }

  return input;
}

void prepareTplArg(std::string &in)
{
  // remove quotes
  in.erase(
    std::remove( in.begin(), in.end(), '\"' ),
    in.end());
}

std::string buildIncludeDirective(
  const std::string& inStr
  , const std::string& quote)
{
  // squarets will generate code from string
  // and append it after annotated variable
  _squaretsString(
R"raw(#include [[+ quote +]][[+ inStr +]][[+ quote +]])raw"
  )
  std::string squarets_output = "";
  return squarets_output;
}

TypeclassCodeGenerator::TypeclassCodeGenerator()
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

TypeclassCodeGenerator::~TypeclassCodeGenerator()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

} // namespace plugin
