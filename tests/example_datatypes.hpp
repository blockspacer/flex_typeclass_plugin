#pragma once

#include <string>

#define GEN_CAT(a, b) GEN_CAT_I(a, b)
#define GEN_CAT_I(a, b) GEN_CAT_II(~, a ## b)
#define GEN_CAT_II(p, res) res

#define GEN_UNIQUE_NAME(base) GEN_CAT(base, __COUNTER__)

// generates class that will have functions with same names
// as in passed data type, forwards calls
#define _typeclass(settings, ...) \
  /* generate definition required to use __attribute__ */ \
  struct \
  __attribute__((annotate("{gen};{funccall};typeclass(" settings ")"))) \
  GEN_UNIQUE_NAME(__gen_tmp__typeclass) \
  : __VA_ARGS__ \
  {};

#define _generate(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
    __attribute__((annotate("{gen};{funccall};" #__VA_ARGS__))) \
    GEN_UNIQUE_NAME(__gen_tmp__apply) \
    ;

#define _generateStr(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
    __attribute__((annotate("{gen};{funccall};" __VA_ARGS__))) \
    GEN_UNIQUE_NAME(__gen_tmp__apply) \
    ;


/// \note example with explicit constructor
struct Square {
  explicit Square(float width, float height)
    : width(width), height(height) {}

  float width;
  float height;
};

/// \note example with explicit constructor
struct CircleTraits {
  explicit Circle(float radius)
    : radius(radius) {}

  float radius;
};

/// \note example with static constexpr member
struct FireSpell {
  std::string title = "FireSpell";
  std::string description = "FireSpell";

  static constexpr char someglobal[] = "someglobal";

  inline friend bool operator<
    (const FireSpell& x, const FireSpell& y)
  {
    return x.title.size() < y.title.size();
  }
};

struct WaterSpell {
  std::string title = "WaterSpell";
  std::string description = "WaterSpell";
};

// like `trait`
struct PrintableTraits {
  virtual void print() const noexcept = 0;

  virtual void some_test_func_proxy
    (const char* arg1) const noexcept = 0;
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
template<typename TypeName1, typename TypeName2>
struct
SummableTraits {
  virtual int sum_with(const TypeName1 arg1, const TypeName2 arg2) const noexcept = 0;
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

template <class ValueType>
struct ForwardIterableTraits {
  //virtual ~ForwardIterableTraits() noexcept = default;
  virtual void operator++() = 0;
  virtual ForwardIterableTraits& clone() const = 0;
};

/// \note example of traits with inheritance
template <class ValueType>
struct BidirectionalIterableTraits
  : public ForwardIterableTraits<ValueType>
{
  //virtual ~BidirectionalIterableTraits() noexcept = default;
  virtual void operator??() = 0;
  virtual BidirectionalIterableTraits& clone() const = 0;
};

/// \note example of traits with operators
struct LessThanComparable<typename T> {
  virtual bool operator<(const T& a, const T& b) = 0;
  /// \note example of traits with default implementation
  virtual bool operator>(const T& a, const T& b) { return b < a; }
  virtual bool operator<=(const T& a, const T& b) { return !(b < a); }
  virtual bool operator>=(const T& a, const T& b) { return !(a < b); }
};

struct SquareTraits {
  virtual void draw() const = 0;
  virtual void set_position(float x, float y) = 0;
  virtual void set_width(float width) = 0;
  virtual void set_height(float height) = 0;
  virtual void set_alpha(float alpha = 0.5) = 0;
};

struct CircleTraits {
  virtual void draw() const = 0;
  virtual void set_position(float x, float y) = 0;
  virtual void set_radius(float radius) = 0;
  virtual void set_alpha(float alpha = 0.3) = 0;
};

/// \todo example of type erasure for function call operator
/// https://github.com/actor-framework/actor-framework/blob/ccd149ed5ae504388ca46c07d5dc43e3a3520938/libcaf_core/caf/detail/unique_function.hpp
