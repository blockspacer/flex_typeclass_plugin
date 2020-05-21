#include <string>
#include <iostream>
#include <vector>

#include <example_datatypes.hpp>

namespace only_for_code_generation {

/// \todo package into conan registry (over internet)
/// \todo dockerize/Vagrantfile conan workspace
/// \todo add CI
/// \todo check on multiple platforms i.e. Windows, Linux
/// \todo add nemespace name and class name to generator settings
/// \todo custom template at runtime
/// \todo rename InHeap to SHARED REMOTE STORAGE
/// \todo rename InPlace to AlwaysLocalStorage
/// \todo add SBO support
/// \todo add shared_ptr support
/// \todo add const ref support by template param TypeclassOptions
/// \todo add default typeclass functions with overriding
/// \todo add default typeclass data members with overriding
/// \todo add templated default typeclass functions
/// \todo add traits inheritance
/// \todo add traits explicit constructor from multiple args
/// \todo add traits operators (==,<,>,+,-,etc.)
/// \todo not templated typeclass functions i.e.
/// NOT has_enough_mana<MagicItem::typeclass>(fs, "spellname");
/// BUT has_enough_mana(fs, "spellname");
/// \todo typeclass functions that can accept typeclass i.e.
/// has_enough_mana(MagicItem{}, "spellname"); // not impl FireSpell, but typeclass MagicItem
/// \todo GMock example
/// \todo example with custom pool allocator
/// \todo debug runtime asserts
/// \todo thread-safety guards
/// \todo use UB sanitizer
/// \todo support for custom vtable http://ldionne.com/cppnow-2018-runtime-polymorphism/#/14/1
/// \todo example with function
/// \todo better unit tests (check crash, compile error messages etc.)
/// \todo benchmarks
/// \todo better docs
/**
template <typename Signature, typename StoragePolicy>
struct basic_function;

template <typename R, typename ...Args, typename StoragePolicy>
struct basic_function<R(Args...), StoragePolicy> {
  template <typename F>
  basic_function(F&& f) : poly_{std::forward<F>(f)} { }

  R operator()(Args ...args) const
  { return poly_.virtual_("call"_s)(poly_, args...); }

private:
  dyno::poly<Callable<R(Args...)>, StoragePolicy> poly_;
};

template <typename Signature>
using function = basic_function<Signature,
                                dyno::sbo_storage<16>>;
template <typename Signature, std::size_t Size = 32>
using inplace_function = basic_function<Signature,
                                        dyno::local_storage<Size>>;
template <typename Signature>
using function_view = basic_function<Signature,
                                     dyno::non_owning_storage>;
template <typename Signature>
using shared_function = basic_function<Signature,
                                       dyno::shared_remote_storage>;
**/

// like `trait`
_typeclass(
  "name = IntSummable"
  ", generator = InPlace"
  ", BufferSize = 64"
  , public SummableTraits<int, int>
)

// like `trait`
_typeclass(
  "name = MagicItem"
  ", generator = InPlace"
  ", BufferSize = 64"
  , public MagicItemTraits
)

// like `trait`
/// \note example of merged typeclasses
/// \note in most cases prefer combined typeclasses to merged
/// \note Merged typeclass combines methods from all provided types.
/// Ensure that inherited method names do not collide.
/// Inherited methods are not optional,
/// you must define them all.
// like `trait`
_typeclass(
  "name = MagicLongType"
  , public MagicTemplatedTraits<std::string, int>
  , public ParentTemplatedTraits_1<const char *>
  , public ParentTemplatedTraits_2<const int &>
)

// like `trait`
_typeclass(
  "name = Printable"
  , public PrintableTraits
)

// like `trait`
_typeclass(
  "name = Spell"
  , public SpellTraits
)

// generates FireSpell_IntSummable
// like impl for trait
// allow typeclass<IntSummableTraits> to store FireSpell
_generate(
  typeclass_instance(
    impl_target = "FireSpell"
    , "IntSummable"
  )
)

// generates int_IntSummable
// like impl for trait
// allow typeclass<IntSummableTraits> to store int
_generate(
  typeclass_instance(
    impl_target = "int"
    , "IntSummable"
  )
)

// Usage: sum int with int
// generates double_IntSummable
// like impl for trait
// allow typeclass<IntSummableTraits> to store double
_generate(
  typeclass_instance(
    impl_target = "double"
    , "IntSummable"
  )
)

// Usage: sum int with double
// generates FireSpell_MagicItem
// like impl for trait
// allow typeclass<MagicItemTraits> to store FireSpell
_generate(
  typeclass_instance(
    impl_target = "FireSpell"
    , "MagicItem"
  )
)

// generates FireSpell_Spell
// like impl for trait
// allow typeclass<SpellTraits> to store FireSpell
_generate(
  typeclass_instance(
    impl_target = "FireSpell"
    , "Spell"
  )
)

// generates FireSpell_Printable
// like impl for trait
// allow typeclass<PrintableTraits> to store FireSpell
_generate(
  typeclass_instance(
    impl_target = "FireSpell"
    , "Printable"
  )
)

// like impl for trait
/// \note example of merged typeclasses
/// all methods are required
///
/// \note in most cases prefer combined typeclasses to merged
/// because combined typeclasses avoids problems releted to
/// collision of function names
// allow typeclass<MagicTemplated...long name> to store FireSpell
_generate(
  typeclass_instance(
    impl_target = "FireSpell"
    , "MagicLongType"
  )
)

// like impl for trait
/// \note you can list multiple functions in same _apply
/// with ';' as delimiter,
_generate(
  typeclass_instance(
    impl_target = "WaterSpell"
    , "Spell"
  )
  ;
  typeclass_instance(
    impl_target = "WaterSpell"
    , "MagicItem"
  )
  ;
  typeclass_instance(
    impl_target = "WaterSpell"
    , "Printable"
  )
)

// like impl for trait
/// \note example of merged typeclasses
/// all methods are required
///
/// \note in most cases prefer combined typeclasses to merged
/// because combined typeclasses avoids problems releted to
/// collision of function names
// allow typeclass<MagicTemplated...long name> to store WaterSpell
_generate(
  typeclass_instance(
    impl_target = "WaterSpell"
    , "MagicLongType"
  )
)

/// \todo remove
// just wraps multiple `traits`, forwards calls
/// \note example of combined typeclasses
// allow typeclass_combo<Spell, MagicItemTraits)>
// to store optional<Spell> and optional<MagicItemTraits>
//_generate(
//  typeclass_combo(
//    name = LongMagicItemSpell
//    , Spell
//    , MagicItem
//    , MagicLongType
//  )
//)

} // namespace only_for_code_generation
