#include <string>
#include <iostream>
#include <vector>

#include <example_datatypes.hpp>

namespace only_for_code_generation {

// like `trait`
$typeclass(
  "name = MagicItem"
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
$typeclass(
  "name = MagicLongType"
  , public MagicTemplatedTraits<std::string, int>
  , public ParentTemplatedTraits_1<const char *>
  , public ParentTemplatedTraits_2<const int &>
)

// like `trait`
$typeclass(
  "name = Printable"
  , public PrintableTraits
)


// like `trait`
$typeclass(
  "name = Spell"
  , public SpellTraits
)

// generates FireSpell_MagicItem
// like impl for trait
// allow typeclass<Spell> to store FireSpell
// allow typeclass<MagicItemTraits> to store FireSpell
// same as:
//$apply(
//  typeclass_instance(target = "FireSpell", "Spell", "MagicItemTraits")
//)
$generate(
  typeclass_instance(
    impl_target = "FireSpell"
    , "MagicItem"
  )
)

// generates FireSpell_Spell
$generate(
  typeclass_instance(
    impl_target = "FireSpell"
    , "Spell"
  )
)

// generates FireSpell_Printable
// like impl for trait
// allow typeclass<Printable> to store FireSpell
$generate(
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
$generate(
  typeclass_instance(
    impl_target = "FireSpell"
    , "MagicLongType"
  )
)

// like impl for trait
/// \note you can list multiple functions in same $apply
/// with ';' as delimiter,
/// it is same as multiple $apply calls
// allow typeclass<Spell, MagicItemTraits> to store WaterSpell
// allow typeclass<Printable> to store WaterSpell
$generate(
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
$generate(
  typeclass_instance(
    impl_target = "WaterSpell"
    , "MagicLongType"
  )
)

// just wraps multiple `traits`, forwards calls
/// \note example of combined typeclasses
// allow typeclass_combo<Spell, MagicItemTraits)>
// to store optional<Spell> and optional<MagicItemTraits>
$generate(
  typeclass_combo(
    name = LongMagicItemSpell
    , Spell
    , MagicItem
    , MagicLongType
  )
)

} // namespace only_for_code_generation
