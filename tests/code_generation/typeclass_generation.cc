#include <string>
#include <iostream>
#include <vector>

#include <example_datatypes.hpp>

namespace only_for_code_generation {

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
