#pragma once

#include <type_traits>
#include <memory>

namespace poly {
namespace generated {

/**
 * TypeclassImplBase is the base class for TypeclassImpl.
 * TypeclassImplBase has a pure virtual function
 * for each method in the interface class typeclass (typename).
 **/
template<typename... typeclass>
struct TypeclassImplBase {
  /**
   * TypeclassImplBase has a virtual dtor
   * to trigger TypeclassImpl's dtor.
   *
   * virtual ~TypeclassImplBase() noexcept { }
   **/

  /**
   * TypeclassImplBase has a virtual clone function
   * to copy-construct an instance of
   * TypeclassImpl into heap memory,
   * which is returned via unique_ptr.
   *
   * virtual std::unique_ptr<TypeclassImplBase>
   *  clone() noexcept = 0;
   **/

  /**
   * virtual std::unique_ptr<TypeclassImplBase>
      move_clone() noexcept = 0;
   **/

  /**
   * virtual std::string
      get_GUID() noexcept = 0;
   **/
};

// TypeclassImpl has the storage for the object of type_t (typename).
template<typename type_t, typename... typeclass>
struct TypeclassImpl : public TypeclassImplBase<typeclass...> {
  typedef type_t type;
};

template<typename... typeclass>
struct Typeclass {
};

template<typename T>
using IsNotReference
  = typename std::enable_if<
      !std::is_reference<T>::value
      , void
    >::type;

} // namespace poly
} // namespace generated
