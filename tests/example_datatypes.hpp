#pragma once

#define GEN_CAT(a, b) GEN_CAT_I(a, b)
#define GEN_CAT_I(a, b) GEN_CAT_II(~, a ## b)
#define GEN_CAT_II(p, res) res

#define GEN_UNIQUE_NAME(base) GEN_CAT(base, __COUNTER__)

// generates class that will have functions with same names
// as in passed data type, forwards calls
#define $typeclass(settings, ...) \
  /* generate definition required to use __attribute__ */ \
  struct \
  __attribute__((annotate("{gen};{funccall};typeclass(" settings ")"))) \
  GEN_UNIQUE_NAME(__gen_tmp__typeclass) \
  : __VA_ARGS__ \
  {};

#define $generate(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
    __attribute__((annotate("{gen};{funccall};" #__VA_ARGS__))) \
    GEN_UNIQUE_NAME(__gen_tmp__apply) \
    ;

#define $generateStr(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
    __attribute__((annotate("{gen};{funccall};" __VA_ARGS__))) \
    GEN_UNIQUE_NAME(__gen_tmp__apply) \
    ;

// combination of multiple typeclasses
// each stored typeclass is optional
// EXAMPLE:
// _tc_combined_t<Spell, MagicItemTraits> tcCombo {
//     FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}
// };
// tcCombo.can_convert<Spell>() // true
// tcCombo.can_convert<MagicItemTraits>() // true
// tcCombo.has_model<Spell>() // true
// tcCombo.has_model<MagicItemTraits>() // false
#define $typeclass_combo(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
    __attribute__((annotate("{gen};{funccall};typeclass_combo" #__VA_ARGS__))) \
    GEN_UNIQUE_NAME(__gen_tmp__typeclass_combo) \
    ;

struct FireSpell {
  std::string title = "FireSpell";
  std::string description = "FireSpell";
};

struct WaterSpell {
  std::string title = "WaterSpell";
  std::string description = "WaterSpell";
};

// like `trait`
struct PrintableTraits {
  virtual void print() const noexcept = 0;
};

// like `trait`
struct SpellTraits {
  virtual void cast(const char* spellname, const int spellpower,
                    const char* target) const noexcept = 0;

  virtual void has_spell(const char* spellname) const noexcept = 0;

  virtual void add_spell(const char* spellname) const noexcept = 0;

  virtual void remove_spell(const char* spellname) const noexcept = 0;

  virtual void set_spell_power(const char* spellname,
                               const int spellpower) const noexcept = 0;
};

// like `trait`
struct
MagicItemTraits {
  virtual void has_enough_mana(const char* spellname) const noexcept = 0;
};

// like `trait`
template<typename T1>
struct
ParentTemplatedTraits_1 {
  virtual void has_P1(T1 name1) const noexcept = 0;
};

// like `trait`
template<typename T1>
struct
ParentTemplatedTraits_2 {
  virtual void has_P2(T1 name1) const noexcept = 0;
};

// like `trait`
template<typename T1, typename T2>
struct
MagicTemplatedTraits {
  virtual void has_T(const T1& name1, const T2& name2) const noexcept = 0;
};
