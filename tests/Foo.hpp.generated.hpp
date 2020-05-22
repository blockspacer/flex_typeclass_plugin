#pragma once

// uncomment if you want to edit codegen code in IDE with autocompletion
#define IDE_CODEGEN_SYNTAX_HIGHLIGHT_HACK 1
#if defined(IDE_CODEGEN_SYNTAX_HIGHLIGHT_HACK)
#include "flexlib/clangUtils.hpp"
#include "flexlib/clangPipeline.hpp"
#include "flexlib/annotation_parser.hpp"
#include "flexlib/annotation_match_handler.hpp"
#include "flexlib/matchers/annotation_matcher.hpp"
#include "flexlib/options/ctp/options.hpp"
#include <cling/Interpreter/Interpreter.h>
#include <llvm/ADT/Optional.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#if defined(CLING_IS_ON)
#include "flexlib/ClingInterpreterModule.hpp"
#endif // CLING_IS_ON
#endif // defined(IDE_CODEGEN_SYNTAX_HIGHLIGHT_HACK)

#include <basis/core/pimpl.hpp>

#include <string>

// exec is similar to executeCodeAndReplace,
// but returns empty source code modification
#define _executeCode(...) \
  /* generate definition required to use __attribute__ */ \
  __attribute__((annotate( \
        "{gen};{executeCodeAndReplace};" \
        #__VA_ARGS__ \
    )))

class Foo {
public:
  Foo();

  ~Foo();

  int foo();

  std::string baz();

private:
  class FooImpl;

#if !defined(FOO_HPP_NO_CODEGEN)
  pimpl::FastPimpl<FooImpl, /*Size*/32, /*Alignment*/8> m_impl;
#endif // !defined(FOO_HPP_NO_CODEGEN)
};
