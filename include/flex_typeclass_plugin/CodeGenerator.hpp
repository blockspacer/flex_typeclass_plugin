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

namespace reflection {

typedef std::string reflectionID;

class ReflectionCXXRecordRegistry {
public:
    ReflectionCXXRecordRegistry(const reflectionID& id,
                                //clang::CXXRecordDecl const *node,
                                ClassInfoPtr classInfoPtr);
    reflectionID id_;
    //clang::CXXRecordDecl const *node_;
    ClassInfoPtr classInfoPtr_;
};

class ReflectionRegistry {
public:
    static ReflectionRegistry *instance;
public:
    static ReflectionRegistry *getInstance();
    std::map<reflectionID, std::unique_ptr<ReflectionCXXRecordRegistry>> reflectionCXXRecordRegistry;
};

} // namespace reflection

namespace MethodPrinter {

namespace Forwarding {

/// \note change only part after `1 <<`
/// 1 << 0, // 00001 == 1
/// 1 << 1, // 00010 == 2
/// 1 << 2, // 00100 == 4
/// 1 << 3, // 01000 == 8
/// 1 << 4, // 10000 == 16
enum Options
{
  FORWARDING_NOTHING = 0
  , EXPLICIT
      = 1 << 1
  , VIRTUAL
      = 1 << 2
  , CONSTEXPR
      = 1 << 3
  , STATIC
      = 1 << 4
  , RETURN_TYPE
      = 1 << 5
  , FORWARDING_ALL
      = MethodPrinter::Forwarding::Options::EXPLICIT
        | MethodPrinter::Forwarding::Options::VIRTUAL
        | MethodPrinter::Forwarding::Options::CONSTEXPR
        | MethodPrinter::Forwarding::Options::STATIC
        | MethodPrinter::Forwarding::Options::RETURN_TYPE
};

} // namespace Forwarding

namespace Trailing {

/// \note change only part after `1 <<`
/// 1 << 0, // 00001 == 1
/// 1 << 1, // 00010 == 2
/// 1 << 2, // 00100 == 4
/// 1 << 3, // 01000 == 8
/// 1 << 4, // 10000 == 16
enum Options
{
  TRAILING_NOTHING = 0
  , CONST
      = 1 << 1
  , NOEXCEPT
      = 1 << 2
  , PURE
      = 1 << 3
  , DELETED
      = 1 << 4
  , DEFAULT
      = 1 << 5
  , BODY
      = 1 << 6
  , TRAILING_ALL
      = MethodPrinter::Trailing::Options::CONST
        | MethodPrinter::Trailing::Options::NOEXCEPT
        | MethodPrinter::Trailing::Options::PURE
        | MethodPrinter::Trailing::Options::DELETED
        | MethodPrinter::Trailing::Options::DEFAULT
        | MethodPrinter::Trailing::Options::BODY
};

} // namespace Trailing

} // namespace MethodPrinter

namespace plugin {

extern const char kSeparatorWhitespace[];

extern const char kSeparatorCommaAndWhitespace[];

extern const char kStructPrefix[];

extern const char kRecordPrefix[];

// name of plugin used in settings KV map
extern const char kSettingsPluginName[];

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

// used by code generator of inline typeclass
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
std::string expandMethodParameterDeclarations(
  const std::vector<reflection::MethodParamInfo>& params);

// input: vector<a, b, c>
// output: "a, b, c"
std::string expandMethodParameterNames(
  const std::vector<reflection::MethodParamInfo>& params);

std::string startHeaderGuard(
  const std::string& guardName);

std::string endHeaderGuard(
  const std::string& guardName);

// we want to generate file names based on parsed C++ types.
// cause file names can not contain spaces ( \n\r)
// and punctuations (,.:') we want to
// replace special characters in filename to '_'
// BEFORE:
//   _typeclass_impl(
//     typeclass_instance(
//       target = "FireSpell",
//       "MagicTemplated<std::string, int>,"
//       "ParentTemplated_1<const char *>,"
//       "ParentTemplated_2<const int &>")
//   )
// AFTER:
//   FireSpell_MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass_instance.generated.hpp
void normalizeFileName(std::string &in);

/// \note prints up to return type
/// (without method name, arguments or body)
/// \note order matters:
/// explicit virtual constexpr static returnType
///   methodName(...) {}
/// \note to disallow some options you can pass
/// something like:
/// (MethodPrinter::Options::FORWARDING_ALL
///  & ~MethodPrinter::Options::EXPLICIT
///  & ~MethodPrinter::Options::VIRTUAL)
/// \note to allow only some options you can pass
/// something like:
/// MethodPrinter::Options::TRAILING_NOTHING
/// | MethodPrinter::Options::CONST
/// | MethodPrinter::Options::NOEXCEPT);
std::string printMethodForwarding(
  const reflection::MethodInfoPtr& methodInfo
  , const std::string& separator = kSeparatorWhitespace
  // what method printer is allowed to print
  // |options| is a bitmask of |MethodPrinter::Options|
  , int options = MethodPrinter::Forwarding::Options::FORWARDING_ALL
);

/// \note order matters:
/// methodName(...)
/// const noexcept override final [=0] [=deleted] [=default]
/// {}
/// \note to disallow some options you can pass
/// something like:
/// (MethodPrinter::Options::TRAILING_ALL
///  & ~MethodPrinter::Options::EXPLICIT
///  & ~MethodPrinter::Options::VIRTUAL)
/// \note to allow only some options you can pass
/// something like:
/// MethodPrinter::Options::TRAILING_NOTHING
/// | MethodPrinter::Options::CONST
/// | MethodPrinter::Options::NOEXCEPT);
std::string printMethodTrailing(
  const reflection::MethodInfoPtr& methodInfo
  , const std::string& separator = kSeparatorWhitespace
  // what method printer is allowed to print
  // |options| is a bitmask of |MethodPrinter::Trailing::Options|
  , int options = MethodPrinter::Trailing::Options::TRAILING_ALL
);

enum class StrJoin
{
  KEEP_LAST_SEPARATOR
  // remove ending separator from NOT empty string
  , STRIP_LAST_SEPARATOR
  , TOTAL
};

std::string joinWithSeparator(
  const std::vector<std::string>& input
  , const std::string& separator
  , StrJoin join_logic
);

// exatracts `SomeType` out of:
// struct SomeType{};
// OR
// class SomeType{};
// Note that `class` in clang LibTooling is `record`
std::string exatractTypeName(
  const std::string& input);

// EXAMPLE:
// _typeclass_impl(
//   typeclass_instance(target = "FireSpell", "Printable")
// )
// Note quotes around "FireSpell":
// target = "FireSpell"
// we want to parse "FireSpell" without quotes
void prepareTplArg(std::string &in);

void forEachDeclaredMethod(
  const std::vector<reflection::MethodInfoPtr>& methods
  , const base::RepeatingCallback<
      void(
        const reflection::MethodInfoPtr&
        , size_t)
    >& func);

std::string buildIncludeDirective(
  const std::string& inStr
  , const std::string& quote = R"raw(")raw");

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
