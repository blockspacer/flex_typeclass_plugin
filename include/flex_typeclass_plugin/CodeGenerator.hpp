#pragma once

#include <flexlib/clangUtils.hpp>
#include <flexlib/ToolPlugin.hpp>
#if defined(CLING_IS_ON)
#include "flexlib/ClingInterpreterModule.hpp"
#endif // CLING_IS_ON

#include <base/logging.h>
#include <base/sequenced_task_runner.h>

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
