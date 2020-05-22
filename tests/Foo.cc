#include "Foo.hpp.generated.hpp"

#include "FooImpl.hpp.generated.hpp"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

Foo::Foo() {
}

Foo::~Foo() {
}

int Foo::foo() {
  return m_impl->foo();
}

std::string Foo::baz() {
  return m_impl->baz();
}
