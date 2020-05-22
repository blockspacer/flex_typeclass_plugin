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
  class
  _executeCode(
    []
    (
      const clang::ast_matchers::MatchFinder::MatchResult& clangMatchResult
      , clang::Rewriter& clangRewriter
      , const clang::Decl* clangDecl
    )
    {
      const std::string typeToMapKey
        = "Foo::FooImpl";

      uint64_t typeSize
        = std::any_cast<uint64_t>(
            global_storage["global_typeSize_for_" + typeToMapKey]);

      unsigned fieldAlign
        = std::any_cast<unsigned>(
            global_storage["global_fieldAlign_for_" + typeToMapKey]);

      std::string fastPimplCode
        = "pimpl::FastPimpl<";

      fastPimplCode += "FooImpl";

      // sizeof(Foo::FooImpl)
      fastPimplCode += ", /*Size*/";
      fastPimplCode += std::to_string(typeSize);

      // alignof(Foo::FooImpl)
      fastPimplCode += ", /*Alignment*/";
      fastPimplCode += std::to_string(fieldAlign);

      fastPimplCode += "> m_impl;";

      /**
       * generates code similar to:
       *  pimpl::FastPimpl<FooImpl, Size, Alignment> m_impl;
       **/
      return new llvm::Optional<std::string>{std::move(fastPimplCode)};
    }(clangMatchResult, clangRewriter, clangDecl);
  ) {}
#endif // !defined(FOO_HPP_NO_CODEGEN)
};
