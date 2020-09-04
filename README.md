# About

Plugin for [https://github.com/blockspacer/flextool](https://github.com/blockspacer/flextool)

Plugin provides support for typeclass-es (or Rust-like traits or Clojure-like protocols or "TEPS" - "Type Erasure Parent Style" or virtual concepts or runtime concepts or Haskell-like type classes or runtime-polymorphic objects with value semantics or inheritance-free polymorphism, etc.).

Note that plugin output is valid C++ code: you can open generated files and debug them as usual.

If you do not know why to use C++ typeclass-es see [https://www.youtube.com/watch?v=OtU51Ytfe04](https://www.youtube.com/watch?v=OtU51Ytfe04)

See for details about flextool [https://blockspacer.github.io/flex_docs/](https://blockspacer.github.io/flex_docs/)

See for more details about typeclass implementation

- http://ldionne.com/cppnow-2018-runtime-polymorphism/#/14/1

See for more details about typeclass-es and `Polymorphic Ducks`:

- [https://mropert.github.io/2017/11/30/polymorphic_ducks/](https://mropert.github.io/2017/11/30/polymorphic_ducks/)
- [https://mropert.github.io/2017/12/17/better_polymorphic_ducks/](https://mropert.github.io/2017/12/17/better_polymorphic_ducks/)
- [https://mropert.github.io/2017/12/23/undefined_ducks/](https://mropert.github.io/2017/12/23/undefined_ducks/)

Runtime Concepts for the C++ Standard Template Library by Sean Parent:

- [https://sean-parent.stlab.cc/papers/2008-03-sac/p171-pirkelbauer.pdf](https://sean-parent.stlab.cc/papers/2008-03-sac/p171-pirkelbauer.pdf)

A Generic, Extendable and Efficient Solution for Polymorphic Programming (p0957r4):

- [http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0957r4.pdf](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0957r4.pdf)

Dynamic Generic Programming with Virtual Concepts by Andrea Proli:

- [https://github.com/andyprowl/virtual-concepts/blob/master/draft/Dynamic%20Generic%20Programming%20with%20Virtual%20Concepts.pdf](https://github.com/andyprowl/virtual-concepts/blob/master/draft/Dynamic%20Generic%20Programming%20with%20Virtual%20Concepts.pdf)

Runtime Polymorphic Generic Programming: Mixing Objects and Concepts in ConceptC++

- [https://pdfs.semanticscholar.org/aa3f/fdcb687f2b5115996f4ef1f2a1ea0a01cb6a.pdf](https://pdfs.semanticscholar.org/aa3f/fdcb687f2b5115996f4ef1f2a1ea0a01cb6a.pdf)

## Before installation

- [installation guide](https://blockspacer.github.io/flex_docs/download/)

## Installation

```bash
export CXX=clang++-6.0
export CC=clang-6.0

# NOTE: change `build_type=Debug` to `build_type=Release` in production
# NOTE: use --build=missing if you got error `ERROR: Missing prebuilt package`
CONAN_REVISIONS_ENABLED=1 \
CONAN_VERBOSE_TRACEBACK=1 \
CONAN_PRINT_RUN_COMMANDS=1 \
CONAN_LOGGING_LEVEL=10 \
GIT_SSL_NO_VERIFY=true \
    cmake -E time \
      conan create . conan/stable \
      -s build_type=Debug -s cling_conan:build_type=Release \
      --profile clang \
          -o flex_typeclass_plugin:enable_clang_from_conan=False \
          -e flex_typeclass_plugin:enable_tests=True
```

## CMake and conan integration

Example code can be found in `flex_typeclass_plugin/tests` directory.

## How it works

Example code can be found in `flex_typeclass_plugin/tests` directory.

0. declare interface what you want to implement

```cpp
struct
MagicItemTraits {
  virtual void has_enough_mana(const char* spellname) const noexcept = 0;
};
``

we want to allow `FireSpell` to be used with `MagicItemTraits` as `MagicItem`

```cpp
struct FireSpell {
  std::string title = "FireSpell";
  std::string description = "FireSpell";
};
```

i.e. we want to do

```cpp
FireSpell myFireSpell{};

// using MagicItem = Typeclass<MagicItemTraits>;
MagicItem tcFireSpell {
  std::move(myFireSpell)
};

tcFireSpell->has_enough_mana("...");
```

`Typeclass<MagicItemTraits>` will be able to store not only `FireSpell` (Inheritance-free polymorphism).

Note that we separated data (`FireSpell`), interface (`MagicItemTraits`) and implementation (see definition of `has_enough_mana` below).

1. generate typeclass

```cpp
// generates typeclass MagicItem
// that must have same functions as
// MagicItemTraits
struct
_typeclass()
MagicItem
  : public MagicItemTraits
{};
```

generates class `Typeclass<MagicItemTraits>` and `using MagicItem = Typeclass<MagicItemTraits>;`

```cpp
// simplified pseudo-code that uses shared_ptr
class Typeclass<MagicItemTraits>
{
  public:
    template <class T>
    Typeclass(T data)
      : self_(std::make_shared<
          // pseudo-code for simplicity
          TypeclassImpl<T,MagicItemTraits>
        >(data)) {}

    // External interface: Just forward the call to the wrapped object.
    void has_enough_mana<MagicItem::typeclass>
      (const char* spellname) const {
        self_->has_enough_mana(spellname);
    }

  private:
    // The abstract base class is hidden under the covers...
    struct TypeclassImplBase<MagicItemTraits>
    {
        virtual ~TypeclassImplBase() = default;
        virtual void has_enough_mana
          (const char* spellname) const = 0;
    };

    // ... and so are the templates.
    template <class T>
    class TypeclassImpl<FireSpell,MagicItemTraits>
      : public TypeclassImplBase<MagicItemTraits>
    {
      public:
        TypeclassImpl(T data) : data_(data) {}
        virtual void has_enough_mana
          (const char* spellname) const override {
            // Forward call
            data_.has_enough_mana(spellname);
        }

      private:
        T data_;
    };

    // in most cases object will be stored not in shared_ptr
    std::shared_ptr<const TypeclassImplBase<MagicItemTraits>> self_;
};
```

`Typeclass<MagicItemTraits>` stores pointer to `TypeclassImplBase<MagicItemTraits>`

`TypeclassImplBase<MagicItemTraits>` will be used as base class.

`TypeclassImplBase` is `Concept` - abstract base class that is hidden under the covers.

```cpp
// will generate files with names based on `MagicItem`:
// 1. MagicItem.typeclass.generated.cpp
// 2. MagicItem.typeclass.generated.hpp
struct
_typeclass()
MagicItem
  : public MagicItemTraits
{};
```

`MagicItem` also generates type alias:

```cpp
using MagicItem = Typeclass<MagicItemTraits>;
```

2. generate typeclass instance

```cpp
// will generate files with names based on `FireSpell_MagicItem`:
// 1. FireSpell_MagicItem.typeclass_instance.generated.cpp
// 2. FireSpell_MagicItem.typeclass_instance.generated.hpp
template<
  typename typeclass_target = MagicItem
  , typename impl_target = FireSpell
>
struct
_typeclass_instance()
FireSpell_MagicItem
{};
```

generates class `TypeclassImpl<FireSpell,MagicItemTraits>`

`TypeclassImpl<FireSpell,MagicItemTraits>` stores `MagicItemTraits` as private member

`TypeclassImpl<FireSpell,MagicItemTraits>` inherits from `TypeclassImplBase<MagicItemTraits>`.

`TypeclassImpl` is `Model` - class that stores data and implements Concept.

3. define functionality related to typeclass instance

```cpp
#include "FireSpell_MagicItem.typeclass_instance.generated.hpp"

namespace morph {
namespace generated {

// allow FireSpell to be used as MagicItemTraits
// MagicItemTraits is base class (typeclass)
template<>
void has_enough_mana<MagicItem::typeclass>
  (const FireSpell& data, const char* spellname) noexcept
{
  std::cout << "(lib1) has_enough_mana " << " by "
    << data.title << " " << spellname << std::endl;
}

} // namespace morph
} // namespace generated
```

where `MagicItem::typeclass` is `InHeapTypeclass<MagicItem>` or `InPlaceTypeclass<MagicItem>` etc. (based on chosen type of code generator)

i.e. we can now do

```cpp
FireSpell myFireSpell{};
Typeclass<MagicItemTraits> tcFireSpell {
  std::move(myFireSpell)
};

tcFireSpell->has_enough_mana("...");
```

what if we need to apply `has_enough_mana` to `FireSpell` without `typeclass` usage?

you can use `has_enough_mana` with `FireSpell` as usual:

```cpp
FireSpell fs;
has_enough_mana<MagicItem::typeclass>(fs, "spellname");
```

## SHARED REMOTE STORAGE

TODO: IN DEVELOPMENT

```cpp
// HOW THAT'S IMPLEMENTED
// pseudo code based on http://ldionne.com/cppnow-2018-runtime-polymorphism/#/8/2
class Vehicle {
  vtable const* const vptr_;
  std::shared_ptr<void> ptr_;

public:
  template <typename Any>
  Vehicle(Any vehicle)
    : vptr_{&vtable_for<Any>}
    , ptr_{std::make_shared<Any>(vehicle)}
  { }

  void accelerate()
  { vptr_->accelerate(ptr_.get()); }
};
```

## NON-OWNING STORAGE (reference semantics, not value semantics)

TODO: IN DEVELOPMENT

```cpp
// HOW THAT'S IMPLEMENTED
// pseudo code based on http://ldionne.com/cppnow-2018-runtime-polymorphism/#/8/2
class VehicleRef {
  vtable const* const vptr_;
  void* ref_;

public:
  template <typename Any>
  VehicleRef(Any& vehicle)
    : vptr_{&vtable_for<Any>}
    , ref_{&vehicle}
  { }

  void accelerate()
  { vptr_->accelerate(ref_); }
};
```

## remote storage

Remote storage is the default one, it always stores a pointer to a heap-allocated object.

## SBO storage

TODO: IN DEVELOPMENT

For example, let's define our drawable wrapper so that it tries to store objects up to 16 bytes in a local buffer, but then falls back to the heap if the object is larger:

## ALWAYS-LOCAL STORAGE

Let's say you actually never want to do an allocation. No problem, just use `generator = InPlace`.

`generator = InPlace` is ALWAYS-LOCAL STORAGE. DOESN'T FIT? DOESN'T COMPILE!

By tweaking these (important) implementation details for you specific use case, you can make your program much more efficient than with classic inheritance.

```cpp
// HOW THAT'S IMPLEMENTED
// pseudo code based on http://ldionne.com/cppnow-2018-runtime-polymorphism/#/8/2
class Vehicle {
  vtable const* const vptr_;
  std::aligned_storage_t<64> buffer_;

public:
  template <typename Any>
  Vehicle(Any vehicle) : vptr_{&vtable_for<Any>} {
    static_assert(sizeof(Any) <= sizeof(buffer_),
      "can't hold such a large object in a Vehicle");
    new (&buffer_) Any(vehicle);
  }

  void accelerate()
  { vptr_->accelerate(&buffer_); }

  ~Vehicle()
  { vptr_->dtor(&buffer_); }
};
```

Use `generator = InPlace` with custom `BufferSize`:

```cpp
// generates typeclass MagicItem
// that must have same functions as
// MagicItemTraits
// We specified `BufferSize = 64` and `generator = InPlace`
// to optimize performance
struct
_typeclass(
  "generator = InPlace"
  ", BufferSize = 64")
MagicLongTypeExample
  : public MagicTemplatedTraits<std::string, int>
{};
```

`generator = InPlace` will generate code that uses aligned storage.

Storage will use the provided size (`BufferSize = 64`)

If storage can not hold provided type, than `static_assert` will raise comilation error (you can see correct size in error message and fix `BufferSize` based on it).

See for details [https://mropert.github.io/2017/12/17/better_polymorphic_ducks/](https://mropert.github.io/2017/12/17/better_polymorphic_ducks/)

## How to configure plugin

Create C++ script that provides function `void loadSettings(Settings& settings)`:

```cpp
namespace flex_typeclass_plugin {

// Declaration must match plugin version.
struct Settings {
  // output directory for generated files
  std::string outDir;
};

void loadSettings(Settings& settings)
{
  settings.outDir
    = "${flextool_outdir}";
}

} // namespace flex_typeclass_plugin
```

See as example `flex_typeclass_plugin/src/flex_typeclass_plugin_settings.cc.in`

flextool can interpret arbitrary C++ code at runtime, just pass command-line argument with path to created C++ script `--cling_scripts=`

```bash
--cling_scripts=${flex_typeclass_plugin_settings}
```

## When to use typeclass-es

Use `Typeclass<MagicItemTraits>` only for polymorphic objects.

Code generated by typeclass can be used both with polymorphic (`Typeclass<MagicItemTraits>`) and with normal objects (`FireSpell fs`).

i.e. for ordinary types can use methods generated by typeclass like so:

```cpp
FireSpell fs;
has_enough_mana<MagicItem::typeclass>(fs, "spellname");
```

This is useful when you don't know beforehand what type of object you will be using.

## Implementation note: typeclass and templated concepts

Our implementation allows to use concepts with templates:

```cpp
template<typename T1, typename T2>
struct
MagicTemplatedTraits {
  virtual void has_T(const T1& name1, const T2& name2) const noexcept = 0;
};

// generates typeclass MagicLongTypeExample
// that must have same functions as
// MagicTemplatedTraits<std::string, int>
struct
_typeclass()
MagicLongTypeExample
  : public MagicTemplatedTraits<std::string, int>
{};

// note that we combined multiple concepts,
// where each concept with `template`
struct
_typeclass()
MagicLongType
  : public MagicTemplatedTraits<std::string, int>
  , public ParentTemplatedTraits_1<const char *>
  , public ParentTemplatedTraits_2<const int &>
{};

// note that we use "MagicLongType" as alias (by `MagicLongType`)
// because without alias type will be too long, like
// FireSpell_MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___
// code below allows to create short file name like
// FireSpell_MagicLongType.typeclass_instance.generated.hpp
template<
  typename typeclass_target = MagicLongType
  , typename impl_target = FireSpell
>
struct
_typeclass_instance()
FireSpell_MagicItem
{};

// implement generated functions somewhere

namespace morph {
namespace generated {

template<>
void has_T<
    MagicLongType::typeclass
  >(const FireSpell& data
  , const std::string &name1
  , const int &name2) noexcept
{
  LOG(WARNING)
    << "(Fire) has_T on " << name1
    << " by " << name2 << " "
    << " by "
    << data.title
    << " ";
}

template<>
void has_P1<
    MagicLongType::typeclass
  >(const FireSpell& data, const char *name1) noexcept
{
  LOG(WARNING)
    << "(FireSpell) has_P1 on " << name1
    << " by "
    << data.title
    << " ";
}

template<>
void has_P2<
    MagicLongType::typeclass
  >(const FireSpell& data, const int& name1) noexcept
{
  LOG(WARNING)
    << "(FireSpell) has_P2 on " << name1
    << " by "
    << data.title
    << " ";
}

} // namespace morph
} // namespace generated

// usage
{
  std::vector<MagicLongType> spellmagicItems;
  {
    MagicLongType pushed{
      FireSpell{"someTmpSpell0", "someTmpSpell0"}};
    spellmagicItems.push_back(std::move(pushed));
  }
  {
    MagicLongType pushed{};
    MagicLongType someTmpSpell{
      FireSpell{"someTmpSpell1", "someTmpSpell1"}};
    pushed = std::move(someTmpSpell); // move
    spellmagicItems.push_back(std::move(pushed));
  }

  for(const MagicLongType& it : spellmagicItems) {
    it.has_P1("p1");
    it.has_T("t0", 1);
  }
}
```

## Implementation note: typeclass-related functions

Most of other typeclass implementations do not use `template<>` to implement typeclass-related functions.

```cpp
// possible issue if two typeclass-es must have function `has_enough_mana`
void has_enough_mana
  (const FireSpell& data, const char* spellname) noexcept
{
  /// ...
}
```

Usage of `template<>` may solve problems related to possible collision of function names in different typeclass-es.

```cpp
// `template` allows to say that logic
// must be implemented only for typeclass `MagicItem`.
template<>
void has_enough_mana<MagicItem::typeclass>
  (const FireSpell& data, const char* spellname) noexcept
{
  /// ...
}
```

That approach is inspired by Rust where you can write code like

```rust
impl MagicItem for FireSpell {
  fn has_enough_mana(&self) {
    // ...
  }
}
```

## How to combine multiple concepts (typeclass-es)

You can find details about that problem at [https://aherrmann.github.io/programming/2014/10/19/type-erasure-with-merged-concepts/](https://aherrmann.github.io/programming/2014/10/19/type-erasure-with-merged-concepts/)

### Approach 1: one model (single model stores whole data)

Merge typeclass-es, use only one model.

If you want to merge typeclass-es `Opener` and `Greeter`, than you can use multiple inheritance:

```cpp
struct Opener {
  virtual void open() const noexcept = 0;
};

struct Greeter {
  virtual void greet() const noexcept = 0;
};

struct
_typeclass()
OpenerAndGreeter
  : public Opener
  , public Greeter
{};
```

And you can use `OpenerAndGreeter` like below:

```cpp
OpenerAndGreeter openerAndGreeter{
  // some data...
};
openerAndGreeter.open();
openerAndGreeter.greet();
```

Pros:
- Good performance
- Good memory usage
- Useful when you want to make each typeclass NOT optional.

Cons:
- Function names from different typeclass-es must not collide.

### Approach 2: multiple optional models (each model stores separate data)

TODO: IN DEVELOPMENT

Merge typeclass-es, use multiple optional models.

```cpp
struct Opener {
  virtual void open() const noexcept = 0;
};

struct Greeter {
  virtual void greet() const noexcept = 0;
};

$typeclass_combination(
  "name = OpenerAndGreeter"
  , public Opener
  , public Greeter
)
```

Allows to make each model of typeclass optional.

For example, `class OpenerAndGreeter` can store:

```cpp
optional<Opener> opener_model;
optional<Greeter> greeter_model;
```

And you can use it like below:

```cpp
OpenerAndGreeter openerAndGreeter;

openerAndGreeter.set<Opener>(
  Opener{
    // some data...
  }
);

if(openerAndGreeter.has<Opener>())
  openerAndGreeter.open<Opener>();

openerAndGreeter.set<Greeter>(
  Greeter{
    // some data...
  }
);

if(openerAndGreeter.has<Greeter>())
  openerAndGreeter.greet<Greeter>();
```

Pros:
- Function names from different typeclass-es can collide.
- Useful when you want to make each typeclass optional.
- Useful when you want to use custom storage type for each typeclass.

Cons:
- Normal performance (must use `has` function before usage of stored typeclass-es)
- Normal memory usage (stores multiple typeclass-es not in single storage)

## Proxy Dilemma

The problem stems from the fact that a referencing type-erasure wrapper is itself a distinct object from the object it erases.

In other words:

```cpp
auto r = Rectangle{{1.0, 2.0}, 5.0, 6.0};
auto s = ShapeRef{std::ref(r)};
assert(&r == &s); // THIS ASSERTION ALWAYS FAILS
```

This is an issue for compile-time generic algorithms written in the form of function templates:

Depending on how they are written, these algorithms may not be allowed to work transparently with objects accessed through a type-erasing wrapper

See for details `Dynamic Generic Programming with Virtual Concepts by Andrea Proli`:

- [https://github.com/andyprowl/virtual-concepts/blob/master/draft/Dynamic%20Generic%20Programming%20with%20Virtual%20Concepts.pdf](https://github.com/andyprowl/virtual-concepts/blob/master/draft/Dynamic%20Generic%20Programming%20with%20Virtual%20Concepts.pdf)

## Design decisions

1. Use template parameters to generate typeclass instance (instead of string with parameters passed as part of annotation attribute)

```cpp
// generates int_IntSummable
// like impl for trait
// allow typeclass<IntSummableTraits> to store int
template<
  typename typeclass_target = IntSummableType
  , typename impl_target = int
>
struct
_typeclass_instance()
int_IntSummable
{};
```

Template parameters require to specify valid type, so typo probability is minimal.

Because template parameter is valid type, we can extract reflection information from it.

That allows to use any valid C++ type as input passed to `_typeclass_instance` and import already generated typeclass from thirparty library.

Ability to import typeclass from thirparty library is important for plugin-based applications.

Note that typeclass (`IntSummable` below) has inner `type` (`IntSummable::type`).

That inner `type` stores information about some settings used during code generation and can be used to import already generated typeclass from thirparty library:

```cpp
/// \note imports existing typeclass (may be from external lib)
using IntSummableType = ::morph::generated::IntSummable::type;

// generates FireSpell_IntSummable
// like impl for trait
// allow typeclass<IntSummableTraits> to store FireSpell
template<
  typename typeclass_target = IntSummableType
  , typename impl_target = FireSpell
>
struct
_typeclass_instance()
FireSpell_IntSummable
{};
```

## Move-only types

If you want to store move-only types, then your interface should have the `bool kIsMoveOnly = true` member variable.

```cpp
// like `trait`
struct
_typeclass()
MagicItem
  : public MagicItemTraits
{
  // To store move-only types
  bool kIsMoveOnly = true;
};
```

```bash
\todo try approach with
https://stackoverflow.com/questions/27073082/conditionally-disabling-a-copy-constructor
std::is_copy_constructible
```

## Development flow (for contributors)

Commands below may be used to build project locally, without system-wide installation.

```bash
export CXX=clang++-6.0
export CC=clang-6.0

cmake -E remove_directory build

cmake -E make_directory build

# NOTE: change `build_type=Debug` to `build_type=Release` in production
build_type=Debug

# install conan requirements
CONAN_REVISIONS_ENABLED=1 \
    CONAN_VERBOSE_TRACEBACK=1 \
    CONAN_PRINT_RUN_COMMANDS=1 \
    CONAN_LOGGING_LEVEL=10 \
    GIT_SSL_NO_VERIFY=true \
        cmake -E chdir build cmake -E time \
            conan install \
            -s build_type=${build_type} -s cling_conan:build_type=Release \
            --build=missing \
            --profile clang \
                -e enable_tests=True \
                ..

# optional: remove generated files (change paths to yours)
rm build/*generated*
rm build/generated/ -rf
rm build/bin/${build_type}/ -rf

# configure via cmake
cmake -E chdir build \
  cmake -E time cmake .. \
  -DENABLE_TESTS=TRUE \
  -DCONAN_AUTO_INSTALL=OFF \
  -DCMAKE_BUILD_TYPE=${build_type}

# build code
cmake -E chdir build \
  cmake -E time cmake --build . \
  --config ${build_type} \
  -- -j8

# run unit tests
cmake -E chdir build \
  cmake -E time cmake --build . \
  --config ${build_type} \
  --target flex_typeclass_plugin_run_all_tests
```

## For contibutors: conan editable mode

With the editable packages, you can tell Conan where to find the headers and the artifacts ready for consumption in your local working directory.
There is no need to run `conan create` or `conan export-pkg`.

See for details [https://docs.conan.io/en/latest/developing_packages/editable_packages.html](https://docs.conan.io/en/latest/developing_packages/editable_packages.html)

Build locally:

```bash
CONAN_REVISIONS_ENABLED=1 \
CONAN_VERBOSE_TRACEBACK=1 \
CONAN_PRINT_RUN_COMMANDS=1 \
CONAN_LOGGING_LEVEL=10 \
GIT_SSL_NO_VERIFY=true \
  cmake -E time \
    conan install . \
    --install-folder local_build \
    -s build_type=Debug -s cling_conan:build_type=Release \
    --profile clang \
      -o flex_typeclass_plugin:enable_clang_from_conan=False \
      -e flex_typeclass_plugin:enable_tests=True

CONAN_REVISIONS_ENABLED=1 \
CONAN_VERBOSE_TRACEBACK=1 \
CONAN_PRINT_RUN_COMMANDS=1 \
CONAN_LOGGING_LEVEL=10 \
GIT_SSL_NO_VERIFY=true \
  cmake -E time \
    conan source . --source-folder local_build

conan build . \
  --build-folder local_build

conan package . \
  --build-folder local_build \
  --package-folder local_build/package_dir
```

Set package to editable mode:

```bash
conan editable add local_build/package_dir \
  flex_typeclass_plugin/master@conan/stable
```

Note that `conanfile.py` modified to detect local builds via `self.in_local_cache`

After change source in folder local_build (run commands in source package folder):

```bash
conan build . \
  --build-folder local_build

conan package . \
  --build-folder local_build \
  --package-folder local_build/package_dir
```

Build your test project

In order to revert the editable mode just remove the link using:

```bash
conan editable remove \
  flex_typeclass_plugin/master@conan/stable
```

## TODOs

TODO: required and optional methods, see https://github.com/seanbaxter/circle/blob/master/erasure/type_erasure.md#specifying-core-and-optional-methods
