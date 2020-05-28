/**
 * It is NOT ordinary header file.
 * It is file with definition used by pimpl pattern.
 **/

#include "FooImpl.hpp.generated.hpp"

int FooImpl::foo() {
  return 1234;
}

std::string FooImpl::baz() {
  return data_;
}

FooImpl::FooImpl() {
}

FooImpl::~FooImpl() {
}
