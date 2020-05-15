#include <string>
#include <iostream>
#include <vector>

#include <example_datatypes.hpp>

// like `trait`
$typeclass(public MagicItem)

// like `trait`
/// \note example of merged typeclasses
/// \note in most cases prefer combined typeclasses to merged
$typeclass(
    public MagicTemplated<std::string, int>
    , public ParentTemplated_1<const char *>
    , public ParentTemplated_2<const int &>)

// like `trait`
$typeclass(public Printable)


// like `trait`
$typeclass(public Spell)


// like impl for trait
// allow typeclass<Spell> to store FireSpell
// allow typeclass<MagicItem> to store FireSpell
// same as:
$apply(
  typeclass_instance(target = "FireSpell", "MagicItem")
)
$apply(
  typeclass_instance(target = "FireSpell", "Spell")
)
//$apply(
//  typeclass_instance(target = "FireSpell", "Spell", "MagicItem")
//)

// like impl for trait
// allow typeclass<Printable> to store FireSpell
$apply(
  typeclass_instance(target = "FireSpell", "Printable")
)

// like impl for trait
/// \note example of merged typeclasses
/// all methods are required
///
/// \note in most cases prefer combined typeclasses to merged
/// because combined typeclasses avoids problems releted to
/// collision of function names
// allow typeclass<MagicTemplated...long name> to store FireSpell
$apply(
  typeclass_instance(
    target = "FireSpell",
    "MagicTemplated<std::string, int>,"
    "ParentTemplated_1<const char *>,"
    "ParentTemplated_2<const int &>")
)


// like impl for trait
/// \note you can list multiple functions in same $apply
/// with ';' as delimiter,
/// it is same as multiple $apply calls
// allow typeclass<Spell, MagicItem> to store WaterSpell
// allow typeclass<Printable> to store WaterSpell
$apply(
  typeclass_instance(target = "WaterSpell",
    "Spell", "MagicItem")
  ;
  typeclass_instance(target = "WaterSpell",
    "Printable")
)

// like impl for trait
/// \note example of merged typeclasses
/// all methods are required
///
/// \note in most cases prefer combined typeclasses to merged
/// because combined typeclasses avoids problems releted to
/// collision of function names
// allow typeclass<MagicTemplated...long name> to store WaterSpell
$apply(
  typeclass_instance(
    target = "WaterSpell",
    "MagicTemplated<std::string, int>,"
    "ParentTemplated_1<const char *>,"
    "ParentTemplated_2<const int &>")
)

// just wraps multiple `traits`, forwards calls
/// \note example of combined typeclasses
// allow typeclass_combo<Spell, MagicItem)>
// to store optional<Spell> and optional<MagicItem>
$typeclass_combo(
  typeclass_combo(Spell, MagicItem)
)
