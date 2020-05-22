#pragma once

#if defined(CODEGEN_RUNNING)
#define FOO_HPP_NO_CODEGEN 1
#include "Foo.hpp"
#else
#include "Foo.hpp.generated.hpp"
#endif

#include <string>

class
_executeCode(
  []
  (
    const clang::ast_matchers::MatchFinder::MatchResult& clangMatchResult
    , clang::Rewriter& clangRewriter
    , const clang::Decl* clangDecl
  )
  {
    clang::SourceManager &SM = clangRewriter.getSourceMgr();

    clang::CXXRecordDecl const *record =
      clangMatchResult.Nodes
        .getNodeAs<clang::CXXRecordDecl>("bind_gen");

    if (!record) {
      CHECK(false);
      return new llvm::Optional<std::string>{};
    }

    if (!record->isCompleteDefinition()) {
      CHECK(false);
      return new llvm::Optional<std::string>{};
    }

    LOG(INFO)
      << "record name is "
      << record->getNameAsString().c_str();

    const clang::ASTRecordLayout& layout
      = clangMatchResult.Context->getASTRecordLayout(record);

    uint64_t typeSize =  layout.getSize().getQuantity();
    // assume it could be a subclass.
    unsigned fieldAlign = layout.getNonVirtualAlignment().getQuantity();

    // If the class is final, then we know that the pointer points to an
    // object of that type and can use the full alignment.
    if (record->hasAttr<clang::FinalAttr>()) {
      fieldAlign = layout.getAlignment().getQuantity();
    }

    const std::string typeToMapKey
      = "Foo::FooImpl";

    global_storage["global_typeSize_for_" + typeToMapKey]
      = typeSize;
    global_storage["global_fieldAlign_for_" + typeToMapKey]
      = fieldAlign;

    LOG(INFO)
      << record->getNameAsString()
      << " Size: " << typeSize
      << " Alignment: " << fieldAlign << '\n';

    return new llvm::Optional<std::string>{};
  }(clangMatchResult, clangRewriter, clangDecl);
)
Foo::FooImpl
{
 public:
  FooImpl();

  ~FooImpl();

  int foo();

  std::string baz();

 private:
  std::string data_{"somedata"};
};
