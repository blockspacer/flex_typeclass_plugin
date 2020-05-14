#pragma once

#define GEN_CAT(a, b) GEN_CAT_I(a, b)
#define GEN_CAT_I(a, b) GEN_CAT_II(~, a ## b)
#define GEN_CAT_II(p, res) res

#define GEN_UNIQUE_NAME(base) GEN_CAT(base, __COUNTER__)

#define $typeclass(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
  __attribute__((annotate("{gen};{funccall};typeclass" ))) \
  GEN_UNIQUE_NAME(__gen_tmp__typeclass) \
  : __VA_ARGS__ \
  {};

#define $typeclass_impl(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
    __attribute__((annotate("{gen};{funccall};" #__VA_ARGS__))) \
    GEN_UNIQUE_NAME(__gen_tmp__typeclass_impl) \
    ;

#define $typeclass_combo(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
    __attribute__((annotate("{gen};{funccall};" #__VA_ARGS__))) \
    GEN_UNIQUE_NAME(__gen_tmp__typeclass_combo) \
    ;

struct FireSpell {
  std::string title = "FireSpell";
  std::string description = "FireSpell";
};

// like impl for trait
struct WaterSpell {
  std::string title = "WaterSpell";
  std::string description = "WaterSpell";
};

// like `trait`
struct Printable {
    virtual void print() const noexcept = 0;
};

// like `trait`
//template<typename S, typename A>
struct Spell {
  virtual void cast(const char* spellname, const int spellpower,
                    const char* target) const noexcept = 0;

  virtual void has_spell(const char* spellname) const noexcept = 0;

  virtual void add_spell(const char* spellname) const noexcept = 0;

  virtual void remove_spell(const char* spellname) const noexcept = 0;

  virtual void set_spell_power(const char* spellname,
                               const int spellpower) const noexcept = 0;

  /// \note same for all types
  // @gen(inject_to_all)
  //S interface_data;
};

struct
MagicItem {
  virtual void has_enough_mana(const char* spellname) const noexcept = 0;

  /// \note same for all types
  // @gen(inject_to_all)
  //S interface_data;
};

template<typename T1>
struct
ParentTemplated_1 {
  virtual void has_P1(T1 name1) const noexcept = 0;
};

template<typename T1>
struct
ParentTemplated_2 {
  virtual void has_P2(T1 name1) const noexcept = 0;
};

template<typename T1, typename T2>
struct
MagicTemplated {
  virtual void has_T(const T1& name1, const T2& name2) const noexcept = 0;

  /// \note same for all types
  // @gen(inject_to_all)
  //S interface_data;
};
