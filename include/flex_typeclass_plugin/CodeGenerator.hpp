#pragma once

#include "flex_typeclass_plugin/flex_typeclass_plugin_settings.hpp"

#include <flexlib/clangUtils.hpp>
#include <flexlib/ToolPlugin.hpp>
#if defined(CLING_IS_ON)
#include "flexlib/ClingInterpreterModule.hpp"
#endif // CLING_IS_ON
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

#include <base/logging.h>
#include <base/sequenced_task_runner.h>

#include <any>
#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>

// provide template code in __VA_ARGS__
/// \note you can use \n to add newline
/// \note does not support #define, #include in __VA_ARGS__
#define _squarets(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" #__VA_ARGS__ )))

// squaretsString
/// \note may use `#include` e.t.c.
// example:
//   _squaretsString("#include <cling/Interpreter/Interpreter.h>")
#define _squaretsString(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" __VA_ARGS__)))

// example:
//   -squaretsFile("file/path/here")
/// \note FILE_PATH can be defined by CMakeLists
/// and passed to flextool via
/// --extra-arg=-DFILE_PATH=...
#define _squaretsFile(...) \
  __attribute__((annotate("{gen};{squaretsFile};CXTPL;" __VA_ARGS__)))

// uses Cling to execute arbitrary code at compile-time
// and run squarets on result returned by executed code
#define _squaretsCodeAndReplace(...) \
  /* generate definition required to use __attribute__ */ \
  __attribute__((annotate("{gen};{squaretsCodeAndReplace};CXTPL;" #__VA_ARGS__)))

namespace plugin {

// output directory for generated files
extern const char kOutDirOption[];

enum class SizePolicy {
  Exact,  // Size == sizeof(T)
  AtLeast // Size >= sizeof(T)
};

enum class AlignPolicy {
  Exact,  // Alignment == alignof(T)
  AtLeast // Alignment >= alignof(T)
};

// used by code generator of inline (in-place) typeclass
struct InlineTypeclassSettings {
  std::string BufferSize
    /// \note empty value means sizeof(T)
    /// in generated code
    = "";

  std::string BufferAlignment
    /// \note empty value means alignof(T)
    /// in generated code
    = "";

  // used by code generator of inline typeclass
  SizePolicy SizePolicyType
    = SizePolicy::AtLeast;

  // used by code generator of inline typeclass
  AlignPolicy AlignPolicyType
    = AlignPolicy::AtLeast;
};

struct TypeclassSettings
{
  bool moveOnly = false;
};

struct TypeclassImplSettings
{
  bool moveOnly = false;
};

// used to prohibit Ctor/Dtor/etc. generation in typeclass
// based on provided interface
bool isTypeclassMethod(
  const reflection::MethodInfoPtr& methodInfo);

// input: vector<a, b, c>
// output: "a, b, c"
std::string expandTemplateTypes(
  const std::vector<reflection::MethodParamInfo>& params);

// input: vector<a, b, c>
// output: "a, b, c"
std::string expandTemplateNames(
  const std::vector<reflection::TemplateParamInfo>& params);

// input: vector<a, b, c>
// output: "a, b, c"
std::string methodParamDecls(
  const std::vector<reflection::MethodParamInfo>& params);

// input: vector<a, b, c>
// output: "a, b, c"
std::string methodParamNames(
  const std::vector<reflection::MethodParamInfo>& params);

// EXAMPLE:
// _typeclass_impl(
//   typeclass_instance(target = "FireSpell", "Printable")
// )
// Note quotes around "FireSpell":
// target = "FireSpell"
// we want to parse "FireSpell" without quotes
void prepareTplArg(std::string &in);

/// \note class name must not collide with
/// class names from other loaded plugins
class TypeclassCodeGenerator {
public:
  TypeclassCodeGenerator();

  ~TypeclassCodeGenerator();

private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TypeclassCodeGenerator);
};

} // namespace plugin
