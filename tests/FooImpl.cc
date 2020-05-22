/**
 * It is NOT ordinary header file.
 * It is file with definition used by pimpl pattern.
 **/

#include "FooImpl.hpp.generated.hpp"

int Foo::FooImpl::foo() {
  return 1234;
}

std::string Foo::FooImpl::baz() {
  return data_;
}

Foo::FooImpl::FooImpl() {
}

Foo::FooImpl::~FooImpl() {
}
