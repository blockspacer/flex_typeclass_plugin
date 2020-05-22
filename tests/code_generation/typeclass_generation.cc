#include <string>
#include <iostream>
#include <vector>

#include <type_erasure_common.hpp>
#include <example_datatypes.hpp>

namespace only_for_code_generation {

/// \todo multiple optional models (each model stores separate data)
/// \todo package into conan registry (over internet)
/// \todo dockerize/Vagrantfile conan workspace
/// \todo add CI
/// \todo check on multiple platforms i.e. Windows, Linux
/// \todo test on clang-cl, msvs and gcc and lower required -std
/// \todo add namespace name and class name to generator settings
/// \todo ADL-style function call generator to support templates
/// \todo better error messages
/// \todo disallow using std::ref(x) in not PolyRef types
/// \todo The type implementing the interface needs to be copyable and movable. (This might change to only require types being movable, when I'm a bit more informed about the use cases.)
/// \todo check with plugin/shared library
/// i.e. add or remove interfaces for arbitrary value types without modifying or recompiling their source code
/// also check for collision problems of definitions - both for static and dynamic lib
/// \todo custom cxtpl template at runtime
/// \todo example with std::transform + std::bind
/// \todo rename InHeap to UNIQUE REMOTE STORAGE
/// \todo rename InPlace
/// name as ExactLocalStorage(size = exactsize, max = maxsize, typeA, typeB)
/// means allocate exactly size, if size is provided
/// if size is not provided - must be able to store typeA, typeB types in-place.
/// if size is not provided - Validates that typeA and typeB have same sizeof.
/// if size is provided - Validates that typeA and typeB have same sizeof as size
/// optionally you can limit maxsize (if typeA, typeB or size exeeded maxsize - do not compile)
/// Note that list of types may be not full (can store any object that have exact size)
/// \todo add SBO support
/// \todo add shared_ptr support, SHARED REMOTE STORAGE
/// \todo add const ref support by template param TypeclassOptions
/// Typeclass<IRegular &>
/// Typeclass<IRegular const &>
/// Typeclass<IFoo &> const anyFoo = foo;
/// Rather than calling member functions with the obj.fun() syntax,
/// you would use the obj->fun() syntax. This is for the sake of const-correctness.
/// operator-> member returns a pointer to the interface with the correct const-ness
/// \todo conversions between non-reference and reference Typeclass-es
/// Poly<IRegular> value = 42;
/// Poly<IRegular &> mutable_ref = value;
/// Poly<IRegular const &> const_ref = mutable_ref;
/// assert(&poly_cast<int>(value) == &poly_cast<int>(mutable_ref));
/// assert(&poly_cast<int>(value) == &poly_cast<int>(const_ref));
/// \todo add default typeclass functions with overriding
/// fall fall back to the default function if no custom function was defined
/// \todo add default typeclass data members with overriding
/// \todo add templated default typeclass functions
/// \todo add traits inheritance
/// \todo close all CXXCTP issues
/// and https://github.com/blockspacer/CXXCTP/issues/22
/// \todo Introspect the wrapped value by asking x.type(). It returns std::type_info const & like typeid(...).)
/// https://github.com/pyrtsa/poly
/// \todo poly::cast<T&&>(std::move(x)) to stored type with BadPolyCast exception
/// \todo poly_cast to stored type with BadPolyCast exception
/// \todo poly_cast<const &> to ref to stored type with BadPolyCast exception
/// \todo If you want to store move-only types, then your interface should extend the poly::IMoveOnly interface.
/// \todo add traits explicit constructor from multiple args
/// \todo add traits operators (==,<,>,+,-,etc.)
/// https://github.com/pyrtsa/poly/blob/master/include/poly/operators.hpp
/// \todo throwings constructor vs poly
/// \todo The Proxy Dilemma (what to move: data or typeclass)
/// https://stlab.cc/legacy/runtime-concepts.html#the-proxy-dilemma
/// Dynamic Generic Programming with Virtual Concepts
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
/// \todo better unit tests (check crash, compile error messages etc.)
/// \todo create example projects
/// \todo benchmarks
/// \todo erasing overloaded operators such
/// as the equality-comparison operator. For example, this is how our ShapeHolder would dispatch
/// equality checks to the underlying object:
/// virtual bool equals(ShapeInternal const& rhs) const override
/// { return this->type_info() == rhs.type_info() &&
///  this->get() == *static_cast<const T*>(rhs.cast()); }
/// \todo remove macro from generated code (remove annotation attribute)
/// \todo implicit type-safe conversions between concepts
/// \todo ShapeRefRef
/// \todo I want to extend typeclass created by lib via typeclass_instance(...)
/// For example, i want to accept not only FireSpell from lib but CustomSpell
/// typeclass(...) generates only new typeclass
/// how to load existing typeclass?
/// lib can provide files used for codegen
/// add LOAD_EXTERNAL param into typeclass(...)
/// UPD: we need only reflection data from SomeTrait, not whole typeclass
/// UPD: include generated typeclass
/// get data types from template args myTypeclass::typeclass
/// now reflect as usual
/// refactor:
/// // lib created using InPlaceSpell
/// template <typename Bases...> class TypeclassGenerator{};
///
///  class my_typeclass_target : public typeclass_target
/// { char name[] = "..."; }
///
/// using my_typeclass_traits = Typeclass<
///     MagicTemplatedTraits<std::string, int>
///     , ParentTemplatedTraits_1<const char *>
///     , ParentTemplatedTraits_2<const int &>>;
///
/// class
/// _typeclass()
/// TypeclassGenerator<my_typeclass_target, my_typeclass_traits
/// >{};
///
/// class
/// typeclass_instance()
/// TypeclassInstanceGenerator
/// <
///   FireSpell_typeclass_instance_target
///   , my_typeclass_traits
///   // OR (from lib) - just extract template args
///   // , public morph::generated::InPlaceSpell
/// >
/// {};
///
/// \todo typeclass to typeclass conversion
/// // ShapeTrait has functions moveToCoordinate(), getX(), getY(), getContourPath()
/// Shape myShape{Rectangle{12.0,21.0,3.1}};
/// // DrawableTrait has functions draw(), draw_debug()
/// Drawable myDrawable{myShape}; // copy
/// Drawable myDrawable{std::move(myShape)}; // move
/// UPD: can be solved via
/// _generate(
///  typeclass_instance(
///    impl_target = "Shape"
///    , "Drawable"
///  )
///)
/// generator knows that both Shape and Drawable are typeclasses and knows functions from each typeclass
/// we can store "Shape" as usual data type in "Drawable"
/// void draw<Shape::typeclass>(const Drawable& data) {
///   somepainter.draw(data.getX(), data.getY(), data.getContourPath())
/// }
/// \todo variant-like polymorphism
/// (automatically max size to max of all sizeof)
/// avoid code bloat in header file (do not specify types or use variant, just change storage size at parse-time)
/// https://www.bfilipek.com/2020/04/variant-virtual-polymorphism.html
/// name as AtLeastLocalStorage(min = minsize, max = maxsize, typeA, typeB)
/// means allocate at least minsize, must be able to store typeA, typeB types in-place
/// optionally you can limit maxsize (if typeA, typeB exeeded maxsize - do not compile)
/// Note that list of types may be not full (can store any object that can be stored in calculated size)
/// \todo inspire by Rust traits
/// https://learning-rust.github.io/docs/b5.impls_and_traits.html#Traits-with-generics
/// https://people.gnome.org/~federico/blog/rust-things-i-miss-in-c.html
/// https://doc.rust-lang.org/book/ch17-02-trait-objects.html
/// \todo move most of the generated code out of hpp
/// \todo public/private/protected inheritance
/// \todo support both code modification b y annotations and new file generation
/// \todo more debug checks:
/// this != &other
/// https://github.com/mikosz/dormouse-engine/blob/07ab12f2aaad2a921299d4e9b8f89c97c9ee8da5/src/foundation/essentials/src/main/c%2B%2B/dormouse-engine/essentials/PolymorphicStorage.hpp#L68
/// \todo per-function selectors to disallow code generation for particular functions
/// \todo error mesage in case of collision of merged function names
/// \todo per-function _optional annotation to add has_funcName()
/// \todo better docs
/// \todo Argument-dependent name lookup is known to be brittle,
/// and you'll likely get problems with ODR (one definition rule)
/// if you aren't careful and end up implementing an interface
/// for some type differently in two compilation units.
/// https://github.com/pyrtsa/poly
/// \todo ability to toggle exceptions or asserts or std::terminate
/// \todo example how to rewrite visitor with type erasure
/// \todo example with function
/// https://github.com/Ladisgin/function/blob/2d1b5b6cee5290bfb9fe5218877c72009ce255a5/function.h#L22
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
