#pragma once

#include <type_traits>
#include <memory>

namespace cxxctp {
namespace generated {

/**
 * _tc_model_t is the base class for _tc_impl_t.
 * _tc_model_t has a pure virtual function
 * for each method in the interface class typeclass (typename).
 **/
template<typename... typeclass>
struct _tc_model_t {
  /**
   * _tc_model_t has a virtual dtor
   * to trigger _tc_impl_t's dtor.
   *
   * virtual ~_tc_model_t() noexcept { }
   **/

  /**
   * _tc_model_t has a virtual clone function
   * to copy-construct an instance of
   * _tc_impl_t into heap memory,
   * which is returned via unique_ptr.
   *
   * virtual std::unique_ptr<_tc_model_t>
   *  clone() noexcept = 0;
   **/

  /**
   * virtual std::unique_ptr<_tc_model_t>
      move_clone() noexcept = 0;
   **/

  /**
   * virtual std::string
      get_GUID() noexcept = 0;
   **/
};

// _tc_impl_t has the storage for the object of type_t (typename).
template<typename type_t, typename... typeclass>
struct _tc_impl_t : public _tc_model_t<typeclass...> {
  typedef type_t val_type_t;
};

template<typename... typeclass>
struct _tc_combined_t {
};

template<typename T>
using IsNotReference
  = typename std::enable_if<
      !std::is_reference<T>::value
      , void
    >::type;

} // namespace cxxctp
} // namespace generated
