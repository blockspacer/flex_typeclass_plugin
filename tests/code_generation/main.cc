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
/// \note example of combined typeclasses, see `typeclass_combo`
$typeclass_impl(
  typeclass_instance(target = "FireSpell", "Spell", "MagicItem")
)

// like impl for trait
/// \note example of combined typeclasses, see `typeclass_combo`
$typeclass_impl(
  typeclass_instance(target = "FireSpell", "Printable")
)

// like impl for trait
/// \note example of merged typeclasses
/// \note in most cases prefer combined typeclasses to merged
$typeclass_impl(
  typeclass_instance(
    target = "FireSpell",
    "MagicTemplated<std::string, int>,"
    "ParentTemplated_1<const char *>,"
    "ParentTemplated_2<const int &>")
)


// like impl for trait
/// \note example of combined typeclasses, see `typeclass_combo`
$typeclass_impl(
  typeclass_instance(target = "WaterSpell",
    "Spell", "MagicItem");
  typeclass_instance(target = "WaterSpell",
    "Printable")
)

// like impl for trait
/// \note example of merged typeclasses
/// \note in most cases prefer combined typeclasses to merged
$typeclass_impl(
  typeclass_instance(
    target = "WaterSpell",
    "MagicTemplated<std::string, int>,"
    "ParentTemplated_1<const char *>,"
    "ParentTemplated_2<const int &>")
)

// just wraps multiple `traits`, forwards calls
/// \note example of combined typeclasses
$typeclass_combo(
  typeclass_combo(Spell, MagicItem)
)

/*int main(int argc, char** argv)
{
  return 0;
}*/
