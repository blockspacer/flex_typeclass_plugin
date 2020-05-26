// This is generated file. Do not modify directly.
// Path to the code generator: /home/avakimov_am/flex_typeclass_plugin/codegen/cxtpl/typeclass/typeclass_gen_hpp.cxtpl.

#pragma once

#include "type_erasure_common.hpp"

#include <array>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstddef>
#include <new>         // launder
#include <type_traits> // aligned_storage
#if !defined(NDEBUG)
#include <cassert>
#endif // NDEBUG

namespace morph {
namespace generated {

  template<
    
    typename T,
    typename V,
    typename std::enable_if<std::is_same<Typeclass<SummableTraits<int, int>>, T>::value>::type* = nullptr
  >
int  sum_with  (const V&, const int arg1, const int arg2  ) noexcept ;


// It is Concept - abstract base class
// that is hidden under the covers.
// Typeclass will store pointer to |implBase_|
// We use |implBase_| as base class for typeclass instance
template<>
struct TypeclassImplBase<SummableTraits<int, int>> {
  TypeclassImplBase() = default;

  // We store a pointer to the base type
  // and rely on TypeclassImplBase's virtual dtor to free the object.
  virtual ~TypeclassImplBase() {}

  virtual
    std::unique_ptr<TypeclassImplBase>
      clone_as_unique_ptr() const = 0;

  virtual
    std::unique_ptr<TypeclassImplBase>
      move_clone_as_unique_ptr() = 0;

virtual int  __sum_with  (
const int arg1, const int arg2  ) const noexcept = 0 ;

};

template<>
struct Typeclass<SummableTraits<int, int>>
{
  // may be used to import existing typeclass
  struct type
    : public SummableTraits<int, int>
  {};

  // use it when you want to implement logic, example:
  //  `void has_enough_mana<MagicItem::typeclass>`
  // using - Type alias, alias template (since C++11)
  using typeclass
    = Typeclass<SummableTraits<int, int>>;

  Typeclass()
    : implBase_{}
  {}

  Typeclass(
    const Typeclass<SummableTraits<int, int>>
      & rhs)
    : implBase_(
        rhs.implBase_->clone_as_unique_ptr())
  {}

  Typeclass(
    Typeclass<SummableTraits<int, int>>
      && rhs)
  {
    implBase_
      = rhs.implBase_->move_clone_as_unique_ptr();
  }

  /// \note use TypeclassRef<ImplType> for references
  template<
    typename ImplType
    /// \note can not pass Typeclass here
    , typename std::enable_if<
        !std::is_same<Typeclass, std::decay_t<ImplType>>::value
      >::type* = nullptr
  >
  Typeclass(ImplType&& impl)
    : implBase_(
        std::make_unique<
          TypeclassImpl<
            std::decay_t<ImplType>
            , SummableTraits<int, int>
          >
        >
        (std::forward<ImplType>(impl)))
  {}

  /// \note use Typeclass<ImplType> for references
  template<
    typename ImplType
    /// \note can not pass Typeclass here
    , typename std::enable_if<
        !std::is_same<Typeclass, std::decay_t<ImplType>>::value
      >::type* = nullptr
  >
  Typeclass& operator=(ImplType&& impl)
  {
    implBase_
      = std::make_unique<
          TypeclassImpl<
            std::decay_t<ImplType>
            , SummableTraits<int, int>
          >
        >
        (std::forward<ImplType>(impl));
    return *this;
  }

  Typeclass<SummableTraits<int, int>>& operator=
    (const Typeclass<SummableTraits<int, int>>
      & rhs)
  {
    implBase_
      = rhs.implBase_->clone_as_unique_ptr();
    return *this;
  }

  Typeclass<SummableTraits<int, int>>& operator=
    (Typeclass<SummableTraits<int, int>>
      && rhs)
  {
    implBase_
      = rhs.implBase_->move_clone_as_unique_ptr();
    return *this;
  }

int  sum_with  (
const int arg1, const int arg2  ) const noexcept   {
    return implBase_->__sum_with
      (arg1, arg2);
  }


private:
  // This is actually a ptr to an impl type.
  std::unique_ptr<
    TypeclassImplBase<SummableTraits<int, int>>
  > implBase_;
};

// It is Concept - abstract base class
// that is hidden under the covers.
/// \note version of typeclass that uses aligned storage
/// instead of unique_ptr
// Typeclass will store pointer to |implBase_|
// We use |implBase_| as base class for typeclass instance
template<>
struct InplaceTypeclassImplBase<SummableTraits<int, int>>
{
  InplaceTypeclassImplBase() = default;

  // We store a pointer to the base type
  // and rely on InplaceTypeclassImplBase's virtual dtor to free the object.
  virtual ~InplaceTypeclassImplBase() {}

  virtual
    InplaceTypeclassImplBase*
      clone_as_raw_ptr(void* addr) const = 0;

  virtual
    InplaceTypeclassImplBase*
      move_clone_as_raw_ptr(void* addr) = 0;

virtual int  __sum_with  (
const int arg1, const int arg2  ) const noexcept = 0 ;

};

/// \note version of typeclass that uses aligned storage
/// instead of unique_ptr
template<>
struct InplaceTypeclass<
    SummableTraits<int, int>
  >
{
  // may be used to import existing typeclass
  struct type
    : public SummableTraits<int, int>
  {};

  // use it when you want to implement logic, example:
  //  `void has_enough_mana<MagicItem::typeclass>`
  // using - Type alias, alias template (since C++11)
  using typeclass
    = Typeclass<SummableTraits<int, int>>;

  InplaceTypeclass()
  {}

  InplaceTypeclass(
    const InplaceTypeclass<
        SummableTraits<int, int>
      >
      & rhs)
  {
#if !defined(_GLIBCXX_HAVE_BUILTIN_LAUNDER)
    implBase_ =
#endif // _GLIBCXX_HAVE_BUILTIN_LAUNDER
      rhs.__implBase()->clone_as_raw_ptr(&storage_);
  }

  InplaceTypeclass(
    InplaceTypeclass<
        SummableTraits<int, int>
      >
      && rhs)
  {
#if !defined(_GLIBCXX_HAVE_BUILTIN_LAUNDER)
    implBase_ =
#endif // _GLIBCXX_HAVE_BUILTIN_LAUNDER
      rhs.__implBase()->move_clone_as_raw_ptr(&storage_);
  }

  /// \note static_assert used only for compile-time checks, so
  /// also don't forget to provide some runtime checks by assert-s
  /// \note we use template,
  /// so compiler will be able to
  /// print required |Size| and |Alignment|
  template<
    std::size_t ActualSize
    , std::size_t ActualAlignment
  >
  constexpr
  inline /* use `inline` to eleminate function call overhead */
  static
  void static_validate() noexcept
  {

      static_assert(
        BufferAlignment >= ActualAlignment
        , "Typeclass: Alignment must be at least alignof(T)");


      static_assert(
        BufferSize >= ActualSize
        , "Typeclass: sizeof(T) must be at least 'Size'");

  }

  /// \note use InplaceTypeclassRef<ImplType> for references
  template<
    typename ImplType
    /// \note can not pass InplaceTypeclass here
    , typename std::enable_if<
        !std::is_same<InplaceTypeclass, std::decay_t<ImplType>>::value
      >::type* = nullptr
  >
  InplaceTypeclass(ImplType&& impl)
  {
    // wrap static_assert into static_validate
    // for console message with desired sizeof in case of error
    static_validate<sizeof(ImplType), alignof(ImplType)>();

#if !defined(_GLIBCXX_HAVE_BUILTIN_LAUNDER)
    implBase_ =
#endif // _GLIBCXX_HAVE_BUILTIN_LAUNDER
      new (&storage_)
        InplaceTypeclassImpl<
            std::decay_t<ImplType>
            , SummableTraits<int, int>
          >(std::forward<ImplType>(impl));
  }

  /// \note use InplaceTypeclassRef<ImplType> for references
  template<
    typename ImplType
    /// \note can not pass InplaceTypeclass here
    , typename std::enable_if<
        !std::is_same<InplaceTypeclass, std::decay_t<ImplType>>::value
      >::type* = nullptr
  >
  InplaceTypeclass& operator=(ImplType&& impl)
  {
    // wrap static_assert into static_validate
    // for console message with desired sizeof in case of error
    static_validate<sizeof(ImplType), alignof(ImplType)>();

#if !defined(NDEBUG)
    assert(__implBase());
#endif // NDEBUG
    __implBase()->
      ~InplaceTypeclassImplBase<SummableTraits<int, int>>();

#if !defined(_GLIBCXX_HAVE_BUILTIN_LAUNDER)
    implBase_ =
#endif // _GLIBCXX_HAVE_BUILTIN_LAUNDER
      new (&storage_)
        InplaceTypeclassImpl<
            std::decay_t<ImplType>
            , SummableTraits<int, int>
          >(std::forward<ImplType>(impl));

    return *this;
  }

  InplaceTypeclass<
    SummableTraits<int, int>
  >& operator=
    (const InplaceTypeclass<
        SummableTraits<int, int>
      >
      & rhs)
  {
#if !defined(NDEBUG)
    assert(__implBase());
#endif // NDEBUG
    __implBase()->
      ~InplaceTypeclassImplBase<SummableTraits<int, int>>();

#if !defined(_GLIBCXX_HAVE_BUILTIN_LAUNDER)
    implBase_ =
#endif // _GLIBCXX_HAVE_BUILTIN_LAUNDER
      rhs.__implBase()->
        clone_as_raw_ptr(&storage_);

    return *this;
  }

  InplaceTypeclass<
    SummableTraits<int, int>
  >& operator=
    (InplaceTypeclass<
      SummableTraits<int, int>
    >
      && rhs)
  {
#if !defined(NDEBUG)
    assert(__implBase());
#endif // NDEBUG
    __implBase()->
      ~InplaceTypeclassImplBase<SummableTraits<int, int>>();

#if !defined(_GLIBCXX_HAVE_BUILTIN_LAUNDER)
    implBase_ =
#endif // _GLIBCXX_HAVE_BUILTIN_LAUNDER
      rhs.__implBase()->
        move_clone_as_raw_ptr(&storage_);

    return *this;
  }

int  sum_with  (
const int arg1, const int arg2  ) const noexcept   {
    return __implBase()->
      __sum_with
      (arg1, arg2);
  }


private:
  inline /* use `inline` to eleminate function call overhead */
  InplaceTypeclassImplBase<SummableTraits<int, int>>*
    __implBase() const noexcept
  {
#if defined(_GLIBCXX_HAVE_BUILTIN_LAUNDER)
    // requires std >= 201703L
    return
      (InplaceTypeclassImplBase<SummableTraits<int, int>>*)
        (std::launder(&storage_));
#else // _GLIBCXX_HAVE_BUILTIN_LAUNDER
    return implBase_;
#endif // _GLIBCXX_HAVE_BUILTIN_LAUNDER
  }

private:
  static constexpr std::size_t BufferSize
    = 64;

  static const size_t BufferAlignment
    = std::alignment_of_v<
        InplaceTypeclassImplBase<SummableTraits<int, int>>
      >;



  /// \note avoid UB related to |aligned_storage_t|
  /// see https://mropert.github.io/2017/12/23/undefined_ducks/
  /// or use std::launder
  /// \note |aligned_storage_t| ensures that memory is contiguous in the class,
  /// avoids cache miss.
  /// (comparing to dynamic heap allocation approach where impl may be in heap,
  /// but the class may be in stack or another region in heap)
  /// \see about `Data Locality` https://gameprogrammingpatterns.com/data-locality.html
  std::aligned_storage_t<
      BufferSize
      , BufferAlignment
    > storage_
    {};

#if !defined(_GLIBCXX_HAVE_BUILTIN_LAUNDER)
  // This is actually a ptr to an impl type.
  /// \note doing a reinterpret_cast from derived to base
  /// is not guaranteed to do what you expect.
  /// A solution to that is to save the result of the new expression
  /// see https://mropert.github.io/2017/12/23/undefined_ducks/
  InplaceTypeclassImplBase<SummableTraits<int, int>>*
    implBase_ = nullptr;
#endif // _GLIBCXX_HAVE_BUILTIN_LAUNDER
};


// using - Type alias, alias template (since C++11)
using InHeapIntSummable
  = Typeclass<SummableTraits<int, int>>;

// using - Type alias, alias template (since C++11)
using InPlaceIntSummable
  = InplaceTypeclass<SummableTraits<int, int>>;


// using - Type alias, alias template (since C++11)
using IntSummable
  = InPlaceIntSummable;


// fullBaseType may be VERY long templated type,
// but we can shorten it with #define
#define DEFINE_IntSummable \
  SummableTraits<int, int>


} // namespace morph
} // namespace generated
