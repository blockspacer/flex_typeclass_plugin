#include <string>
#include <iostream>
#include <vector>

#include <example_datatypes.hpp>

// like `trait`
struct
__attribute__((annotate("{gen};{funccall};typeclass(public MagicItemTraits, name = MagicItem)" )))
GEN_UNIQUE_NAME(__gen_tmp__typeclass)
: public MagicItemTraits
{};

// like `trait`
/// \note example of merged typeclasses
/// \note in most cases prefer combined typeclasses to merged
struct
__attribute__((annotate("{gen};{funccall};typeclass(public MagicTemplated<std::string, int>, public ParentTemplated_1<const char *>, public ParentTemplated_2<const int &>, name = MagicLongType)" )))
GEN_UNIQUE_NAME(__gen_tmp__typeclass)
: public MagicTemplated<std::string, int>
  , public ParentTemplated_1<const char *>
  , public ParentTemplated_2<const int &>
{};

// like `trait`
$typeclass(public Printable)


// like `trait`
$typeclass(public Spell)

// like impl for trait
// allow typeclass<Spell> to store FireSpell
// allow typeclass<MagicItemTraits> to store FireSpell
// same as:
//$apply(
//  typeclass_instance(target = "FireSpell", "Spell", "MagicItemTraits")
//)
$generate(
  typeclass_instance(
    target = "FireSpell"
    , type = "MagicItemTraits"
    , "MagicItem")
)
$generate(
  typeclass_instance(target = "FireSpell", "Spell")
)

// like impl for trait
// allow typeclass<Printable> to store FireSpell
$generate(
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
$generate(
  typeclass_instance(
    target = "FireSpell"
    , type = "MagicTemplated<std::string, int>,"
             "ParentTemplated_1<const char *>,"
             "ParentTemplated_2<const int &>"
    , "MagicLongType")
)

// like impl for trait
/// \note you can list multiple functions in same $apply
/// with ';' as delimiter,
/// it is same as multiple $apply calls
// allow typeclass<Spell, MagicItemTraits> to store WaterSpell
// allow typeclass<Printable> to store WaterSpell
$generate(
  typeclass_instance(target = "WaterSpell"
    , "Spell")
  ;
  typeclass_instance(target = "WaterSpell"
    , type = "MagicItemTraits"
    , "MagicItem")
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
//$generate(
//  typeclass_instance(
//    target = "WaterSpell"
//    , "MagicTemplated<std::string, int>,"
//    "ParentTemplated_1<const char *>,"
//    "ParentTemplated_2<const int &>")
//)
// \todo support for short alias
$generate(
  typeclass_instance(
    target = "WaterSpell"
    , type = "MagicTemplated<std::string, int>,"
             "ParentTemplated_1<const char *>,"
             "ParentTemplated_2<const int &>"
    , "MagicLongType")
)

// just wraps multiple `traits`, forwards calls
/// \note example of combined typeclasses
// allow typeclass_combo<Spell, MagicItemTraits)>
// to store optional<Spell> and optional<MagicItemTraits>
$generate(
  typeclass_combo(Spell,
    MagicItem)
)
