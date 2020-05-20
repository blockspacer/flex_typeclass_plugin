#pragma once

#include <type_traits>
#include <memory>

namespace poly {
namespace generated {

/* Required wrapper for if constexpr
 *
 * Is dependent on a template parameter.
 * Is used in static_assert in a false branch to produce a compile error
 * with message containing provided type.
 * See an example with dependent_false at https://en.cppreference.com/w/cpp/language/if
 *
 * if constexpr (std::is_same<T, someType1>) {
 * } else if constexpr (std::is_same<T, someType2>) {
 * } else {
 *     static_assert(dependent_false<T>::value, "unknown type");
 * }
 */
template<class U>
struct dependent_false : std::false_type {};

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

/// \note version of typeclass that uses aligned storage
/// instead of unique_ptr
template<typename... typeclass>
struct InplaceTypeclassImplBase {
};

// TypeclassImpl has the storage for the object of type_t (typename).
template<typename type_t, typename... typeclass>
struct TypeclassImpl : public TypeclassImplBase<typeclass...> {
  typedef type_t type;
};

/// \note version of typeclass that uses aligned storage
/// instead of unique_ptr
template<typename type_t, typename... typeclass>
struct InplaceTypeclassImpl : public TypeclassImplBase<typeclass...> {
  typedef type_t type;
};

template<typename... typeclass>
struct Typeclass {
};

/// \note version of typeclass that uses aligned storage
/// instead of unique_ptr
template<
  typename... typeclass
>
struct InplaceTypeclass {
};

template<typename T>
using IsNotReference
  = typename std::enable_if<
      !std::is_reference<T>::value
      , void
    >::type;

} // namespace poly
} // namespace generated
