# About

Plugin for [https://github.com/blockspacer/flextool](https://github.com/blockspacer/flextool)

Plugin provides support for typeclasses (or Rust-like traits or "TEPS" - "Type Erasure Parent Style" or virtual concepts).

If you don`t know why to use C++ typeclasses see https://www.youtube.com/watch?v=OtU51Ytfe04

See for details [https://blockspacer.github.io/flex_docs/](https://blockspacer.github.io/flex_docs/)

See for more details about typeclasses and `Polymorphic Ducks`:

- https://mropert.github.io/2017/11/30/polymorphic_ducks/
- https://mropert.github.io/2017/12/17/better_polymorphic_ducks/
- https://mropert.github.io/2017/12/23/undefined_ducks/

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

```
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

## How it works

0. declare interface what you want to implement

```cpp
struct
MagicItemTraits {
  virtual void has_enough_mana(const char* spellname) const noexcept = 0;
};
``

we want to allow `FireSpell` to be used with `MagicItemTraits` as `MagicItem`

```cpp
// like impl for trait
struct FireSpell {
  std::string title = "FireSpell";
  std::string description = "FireSpell";
};
```

i.e. we want to do

```cpp
FireSpell myFireSpell{};

// using MagicItem = _tc_combined_t<MagicItemTraits>;
MagicItem tcFireSpell {
  std::move(myFireSpell)
};

tcFireSpell->has_enough_mana("...");
```

`_tc_combined_t<MagicItemTraits>` will be able to store not only `FireSpell`.

1. generete typeclass

```cpp
_typeclass(public MagicItemTraits, name = MagicItem)
```

generates class `_tc_combined_t<MagicItemTraits>`

`_tc_combined_t<MagicItemTraits>` stores pointer to `_tc_model_t<MagicItemTraits>`

`_tc_model_t<MagicItemTraits>` will be used as base class

`name` parameter is optional, it generates

```cpp
using MagicItem = _tc_combined_t<MagicItemTraits>;
```

also `name` parameter controls name of generated `.hpp` and `.cpp` files.

2. generete typeclass instance

```cpp
_apply(
  typeclass_instance(target = "FireSpell", "MagicItemTraits")
)
```

generates class `_tc_impl_t<FireSpell,MagicItemTraits>`

`_tc_impl_t<FireSpell,MagicItemTraits>` stores `MagicItemTraits` as private member

`_tc_impl_t<FireSpell,MagicItemTraits>` inherits from `_tc_model_t<MagicItemTraits>`

3. define functionality related to typeclass instance

```cpp
#include "generated/FireSpell_MagicItemTraits.typeclass_instance.generated.hpp"

namespace poly {
namespace generated {

// allow FireSpell to be used as MagicItemTraits
// MagicItemTraits is base class (typeclass)
template<>
void has_enough_mana<MagicItemTraits, FireSpell>
    (const FireSpell& data, const char* spellname) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) has_enough_mana " << " by "
      << data.title << " " << spellname << std::endl;
}

} // namespace poly
} // namespace generated
```

i.e. we can now do

```cpp
FireSpell myFireSpell{};
_tc_combined_t<MagicItemTraits> tcFireSpell {
  std::move(myFireSpell)
};

tcFireSpell->has_enough_mana("...");
```

what if we need to apply `has_enough_mana` to `FireSpell` without `typeclass` usage?

you can use `has_enough_mana` with `FireSpell` as usual:

```cpp
FireSpell fs;
has_enough_mana<MagicItemTraits, FireSpell>(fs, "spellname");
```

## When to use typeclasses

Use `_tc_combined_t<MagicItemTraits>` only for polymorphic objects.

Code generated by typeclass can be used both with polymorphic (`_tc_combined_t<MagicItemTraits>`) and with normal objects (`FireSpell fs`).

i.e. for ordinary types can use methods generated by typeclass like so:

```cpp
FireSpell fs;
has_enough_mana<MagicItemTraits, FireSpell>(fs, "spellname");
```

This is useful when you don't know beforehand what type of object you will be using.
