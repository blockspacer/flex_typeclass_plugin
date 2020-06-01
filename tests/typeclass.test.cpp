#include "testsCommon.h"

#if !defined(USE_GTEST_TEST)
#warning "use USE_GTEST_TEST"
// default
#define USE_GTEST_TEST 1
#endif // !defined(USE_GTEST_TEST)

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

#include "flexlib/annotation_match_handler.hpp"
#include "flexlib/annotation_parser.hpp"
#include "flexlib/matchers/annotation_matcher.hpp"
#include "flexlib/clangPipeline.hpp"
#include "flexlib/parser_constants.hpp"

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/path_service.h>
#include <base/base64.h>
#include <base/base64url.h>
#include <base/command_line.h>
#include <base/lazy_instance.h>
#include <base/logging.h>
#include <base/trace_event/trace_event.h>
#include <base/cpu.h>
#include <base/optional.h>
#include <base/path_service.h>
#include <base/time/time.h>
#include <base/memory/ptr_util.h>
#include <base/macros.h>
#include <base/stl_util.h>
#include <base/bind.h>
#include <base/bind_helpers.h>
#include <base/memory/scoped_refptr.h>
#include <base/single_thread_task_runner.h>
#include <base/threading/thread_task_runner_handle.h>
#include <base/numerics/checked_math.h>
#include <base/numerics/clamped_math.h>
#include <base/numerics/safe_conversions.h>
#include <base/i18n/icu_string_conversions.h>
#include <base/i18n/string_compare.h>
#include <base/stl_util.h>
#include <base/base_switches.h>
#include <base/command_line.h>
#include <base/containers/small_map.h>
#include <base/i18n/icu_util.h>
#include <base/i18n/rtl.h>
#include <base/allocator/partition_allocator/page_allocator.h>
#include <base/allocator/allocator_shim.h>
#include <base/allocator/buildflags.h>
#include <base/allocator/partition_allocator/partition_alloc.h>
#include <base/memory/scoped_refptr.h>
#include <base/i18n/rtl.h>
#include <base/stl_util.h>
#include <base/memory/ref_counted_memory.h>
#include <base/memory/read_only_shared_memory_region.h>
#include <base/stl_util.h>
#include <base/bind.h>
#include <base/memory/weak_ptr.h>
#include <base/threading/thread.h>
#include <base/logging.h>
#include <base/system/sys_info.h>
#include <base/synchronization/waitable_event.h>
#include <base/observer_list.h>
#include <base/synchronization/lock.h>
#include <base/threading/thread.h>
#include <base/files/file_path.h>
#include <base/strings/string_util.h>
#include <base/timer/timer.h>
#include <base/callback.h>
#include <base/bind.h>
#include <base/callback.h>
#include <base/command_line.h>
#include <base/logging.h>
#include <base/memory/weak_ptr.h>
#include <base/message_loop/message_loop.h>
#include <base/optional.h>
#include <base/bind.h>
#include <base/callback.h>
#include <base/files/file_path.h>
#include <base/message_loop/message_loop.h>
#include <base/threading/thread.h>
#include <base/threading/thread_checker.h>
#include <base/feature_list.h>
#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/path_service.h>
#include <base/at_exit.h>
#include <base/command_line.h>
#include <base/message_loop/message_loop.h>
#include <base/run_loop.h>
#include <base/trace_event/trace_event.h>
#include <base/trace_event/trace_buffer.h>
#include <base/trace_event/trace_log.h>
#include <base/trace_event/memory_dump_manager.h>
#include <base/trace_event/heap_profiler.h>
#include <base/trace_event/heap_profiler_allocation_context_tracker.h>
#include <base/trace_event/heap_profiler_event_filter.h>
#include <base/sampling_heap_profiler/sampling_heap_profiler.h>
#include <base/sampling_heap_profiler/poisson_allocation_sampler.h>
#include <base/trace_event/malloc_dump_provider.h>
#include <base/trace_event/memory_dump_provider.h>
#include <base/trace_event/memory_dump_scheduler.h>
#include <base/trace_event/memory_infra_background_whitelist.h>
#include <base/trace_event/process_memory_dump.h>
#include <base/trace_event/trace_event.h>
#include <base/allocator/allocator_check.h>
#include <base/base_switches.h>
#include <base/threading/sequence_local_storage_map.h>
#include <base/command_line.h>
#include <base/debug/alias.h>
#include <base/debug/stack_trace.h>
#include <base/memory/ptr_util.h>
#include <base/sequenced_task_runner.h>
#include <base/threading/thread.h>
#include <base/threading/thread_task_runner_handle.h>
#include <base/process/process_handle.h>
#include <base/single_thread_task_runner.h>
#include <base/task_runner.h>
#include <base/task_runner_util.h>
#include <base/task/thread_pool/thread_pool.h>
#include <base/task/thread_pool/thread_pool_impl.h>
#include <base/threading/thread.h>
#include <base/threading/thread_local.h>
#include <base/metrics/histogram.h>
#include <base/metrics/histogram_macros.h>
#include <base/metrics/statistics_recorder.h>
#include <base/metrics/user_metrics.h>
#include <base/metrics/user_metrics_action.h>
#include <base/threading/platform_thread.h>

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include "example_datatypes.hpp"

//#include "LongMagicItemSpell.typeclass_combo.generated.hpp"

#include <flex_typeclass_plugin/pregenerated/IntSummable.typeclass.generated.hpp>
//#include "IntSummable.typeclass.generated.hpp"

#include "Spell.typeclass.generated.hpp"
#include "MagicLongType.typeclass.generated.hpp"
#include "Printable.typeclass.generated.hpp"
#include "MagicItem.typeclass.generated.hpp"

#include "FireSpell_IntSummable.typeclass_instance.generated.hpp"
#include "int_IntSummable.typeclass_instance.generated.hpp"
#include "double_IntSummable.typeclass_instance.generated.hpp"

#include "FireSpell_MagicItem.typeclass_instance.generated.hpp"

#include "WaterSpell_MagicItem.typeclass_instance.generated.hpp"

#include "FireSpell_MagicLongType.typeclass_instance.generated.hpp"

#include "WaterSpell_MagicLongType.typeclass_instance.generated.hpp"

#include "FireSpell_Printable.typeclass_instance.generated.hpp"

namespace morph {
namespace generated {

// allow FireSpell to be used as MagicItemTraits
// MagicItemTraits is base class (typeclass)
template<>
void has_enough_mana<MagicItem::typeclass>
  (const FireSpell& data, const char* spellname) noexcept
{
  LOG(INFO)
    << "(lib1) has_enough_mana " << " by "
    << data.title << " " << spellname;
  LOG(INFO)
    << "someglobal " << data.someglobal;
}

} // namespace morph
} // namespace generated

namespace morph {
namespace generated {

// allow WaterSpell to be used as MagicItemTraits
// MagicItemTraits is base class (typeclass)
template<>
void has_enough_mana<Typeclass<DEFINE_MagicItem>::typeclass>
  (const WaterSpell& data, const char* spellname) noexcept
{
  LOG(INFO)
    << "(lib1) has_enough_mana " << " by "
    << data.title << " " << spellname;
}

} // namespace morph
} // namespace generated

namespace morph {
namespace generated {

template<>
void has_T<
    MagicLongType::typeclass
  >(const FireSpell& data
  , const std::string &name1
  , const int &name2) noexcept
{
  LOG(INFO)
    << "(Fire) has_T on " << name1
    << " by " << name2 << " "
    << " by "
    << data.title
    << " ";
  LOG(INFO)
    << "someglobal " << data.someglobal;
}

template<>
void has_P1<
    MagicLongType::typeclass
  >(const FireSpell& data, const char *name1) noexcept
{
  LOG(INFO)
    << "(FireSpell) has_P1 on " << name1
    << " by "
    << data.title
    << " ";
  LOG(INFO)
    << "someglobal " << data.someglobal;
}

template<>
void has_P2<
    MagicLongType::typeclass
  >(const FireSpell& data, const int& name1) noexcept
{
  LOG(INFO)
    << "(FireSpell) has_P2 on " << name1
    << " by "
    << data.title
    << " ";
  LOG(INFO)
    << "someglobal " << data.someglobal;
}

} // namespace morph
} // namespace generated

namespace morph {
namespace generated {

template<>
void has_T<
    MagicLongType::typeclass
  >(const WaterSpell& data
  , const std::string &name1
  , const int &name2) noexcept
{
  LOG(INFO)
    << "(WaterSpell) has_T on "
    << name1
    << " by "
    << data.title
    << " ";
}

template<>
void has_P1<
    MagicLongType::typeclass
  >(const WaterSpell& data, const char *name1) noexcept
{
  LOG(INFO)
    << "(WaterSpell) has_P1 on " << name1
    << " by "
    << data.title
    << " ";
}

template<>
void has_P2<
    MagicLongType::typeclass
  >(const WaterSpell& data, const int& name1) noexcept
{
  LOG(INFO)
    << "(WaterSpell) has_P2 on " << name1;
}

} // namespace morph
} // namespace generated

namespace morph {
namespace generated {

template<>
void print<Printable::typeclass>
  (const FireSpell& data) noexcept
{
  LOG(INFO)
    << "(lib1) print for FireSpell "
    << data.title << " " << data.description;
  LOG(INFO)
    << "someglobal " << data.someglobal;
}

} // namespace morph
} // namespace generated

#include "WaterSpell_Printable.typeclass_instance.generated.hpp"

namespace morph {
namespace generated {

template<>
void print<Printable::typeclass>
  (const WaterSpell& data) noexcept
{
  LOG(INFO)
    << "(lib1) print for WaterSpell "
    << data.title << " " << data.description;
}

} // namespace morph
} // namespace generated

#include "FireSpell_Spell.typeclass_instance.generated.hpp"

namespace morph {
namespace generated {

template<>
void cast<Spell::typeclass>
  (const FireSpell& data, const char* spellname, const int spellpower,
   const char* target) noexcept
{
  LOG(INFO)
    << "(lib1) cast on " << target
    << " by " << data.title << " " << spellname
    << " with " << spellpower;
  LOG(INFO)
    << "someglobal " << data.someglobal;
}

template<>
void has_spell<Spell::typeclass>(
  const FireSpell& data, const char *spellname ) noexcept
{
  LOG(INFO)
    << "(lib1) has_spell by "
    << data.title << " " << spellname
    << " with " << spellname;
  LOG(INFO)
    << "someglobal " << data.someglobal;
}

template<>
void add_spell<Spell::typeclass>(
  const FireSpell& data
  , const char *spellname ) noexcept
{
  LOG(INFO)
    << "(lib1) add_spell by " << data.title << " " << spellname
    << " with " << spellname;
  LOG(INFO)
    << "someglobal " << data.someglobal;
}

template<>
void remove_spell<Spell::typeclass>(
  const FireSpell& data
  , const char *spellname ) noexcept
{
  LOG(INFO)
    << "(lib1) remove_spell by " << data.title << " " << spellname
    << " with " << spellname;
  LOG(INFO)
    << "someglobal " << data.someglobal;
}

template<>
void set_spell_power<Spell::typeclass>
    (const FireSpell& data
    , const char *spellname
    , const int spellpower) noexcept
{
  LOG(INFO)
    << "(lib1) set_spell_power by " << data.title << " " << spellname
    << " with " << spellpower;
}

} // namespace morph
} // namespace generated

#include "WaterSpell_Spell.typeclass_instance.generated.hpp"

void some_test_func
  (WaterSpell& data, const char* arg1) noexcept;

namespace morph {
namespace generated {

template<>
void cast<Spell::typeclass>
  (const WaterSpell& data
  , const char* spellname
  , const int spellpower,
   const char* target) noexcept
{
  LOG(INFO)
    << "(lib1) cast on " << target
    << " by " << data.title << " " << spellname
    << " with " << spellpower;
}

template<>
void has_spell<Spell::typeclass>(
  const WaterSpell& data, const char *spellname ) noexcept
{
  LOG(INFO)
    << "(lib1) has_spell by "
    << data.title << " " << spellname
    << " with " << spellname;
}

template<>
void add_spell<Spell::typeclass>(
  const WaterSpell& data, const char *spellname ) noexcept
{
  LOG(INFO)
    << "(lib1) add_spell by " << data.title << " " << spellname
    << " with " << spellname;
}

template<>
void remove_spell<Spell::typeclass>(
  const WaterSpell& data, const char *spellname ) noexcept
{
  LOG(INFO)
    << "(lib1) remove_spell by " << data.title << " " << spellname
    << " with " << spellname;
}

template<>
void set_spell_power<Spell::typeclass>
  (const WaterSpell& data
  , const char *spellname
  , const int spellpower) noexcept
{
  LOG(INFO)
    << "(lib1) set_spell_power by " << data.title << " " << spellname
    << " with " << spellpower;
}

template<>
void some_test_func_proxy<Printable::typeclass>
  (const WaterSpell& data, const char* arg1) noexcept
{
  some_test_func(
    /// \todo type erasure without const_cast
    //data
    const_cast<WaterSpell&>(data)
    , arg1);
}

/// \note optional
//template<>
//void some_test_func_proxy<Printable::typeclass>
//  (const FireSpell& data, const char* arg1) noexcept
//{
//  LOG(INFO)
//    << "(lib1) print for FireSpell "
//    << data.title << " " << data.description;
//}

} // namespace morph
} // namespace generated

namespace morph {
namespace generated {

template<>
int sum_with<IntSummable::typeclass>
  (const FireSpell& data, const int arg1, const int arg2) noexcept
{
  return data.title.size() + arg1 + arg2;
}

template<>
int sum_with<IntSummable::typeclass>
  (const int& data, const int arg1, const int arg2) noexcept
{
  return data + arg1 + arg2;
}

template<>
int sum_with<IntSummable::typeclass>
  (const double& data, const int arg1, const int arg2) noexcept
{
  return data + arg1 + arg2;
}

/// \todo
//template <>
//template <
//  typename T
//  /*, typename std::enable_if<
//    !std::is_same<IntSummable::typeclass, std::decay_t<T>>::value
//  >::type* = nullptr*/
//>
//int sum_with<IntSummable::typeclass, T>
//  (const T& data, const int arg1, const int arg2) noexcept
//{
//  return data + arg1 + arg2;
//}

/// \todo
//template <typename A, typename B>
//auto call(add_, A const & a, B const & b)
//  -> decltype(a + b) { return a + b; }

/// \todo
// template <typename A, typename B, typename... More>
// auto call(add_, A const & a, B const & b, More const &... more)
//  -> decltype(ns::add(a + b, more...)) { return ns::add(a + b, more...); }

} // namespace morph
} // namespace generated

void some_test_func
  (WaterSpell& data, const char* arg1) noexcept
{
  data.title = "changed title";

  LOG(INFO)
    << "some_test_func "
    << " by "
    << data.title
    << " "
    << arg1;
}

void pass_by_cref
  (const morph::generated::Printable& printable
   , const char* arg1) noexcept
{
  /// \todo disallow modification of
  /// const morph::generated::Printable& printable
  /// create ConstRefPrintable
  printable.some_test_func_proxy(arg1);
}

// USAGE:
// int elem = random elem(v.begin(), v.end()); // v is a vector of int
// template<typename ForwardIterable<ForwardIterableType>>
//ForwardIterable<ForwardIterableType>
//  getRandomElem
//    (ForwardIterable<ForwardIterableType> first, ForwardIterable<ForwardIterableType> last)
//{
//  ForwardIterable<ForwardIterableType>::difference_type dist
//    = std::distance(first, last);
//  return std::advance(f, rand() % dist);
//}

TEST(Typeclass, TypeclassGeneration) {
  using namespace morph::generated;

  /// \note not initializeded typeclass will crash
  //{
  //  InplaceTypeclass<SpellTraits> myspell;
  //  myspell.set_spell_power(
  //    /*spellname*/ "spellname4"
  //    , /*spellpower*/ 444);
  //}

  /// \todo
  /// std::sort(vehicles.begin(), vehicles.end());

  /// \todo
  /// struct Vehicle {
  ///  vtable const* const vptr_;
  ///  union { void* ptr_;
  ///          std::aligned_storage_t<16> buffer_; };
  ///  bool on_heap_;
  ///
  ///  template <typename Any>
  ///  Vehicle(Any vehicle) : vptr_{&vtable_for<Any>} {
  ///    if constexpr (sizeof(Any) > 16) {
  ///      on_heap_ = true;
  ///      ptr_ = new Any(vehicle);
  ///    } else {
  ///      on_heap_ = false;
  ///      new (&buffer_) Any{vehicle};
  ///    }
  ///  }
  ///
  ///  void accelerate()
  ///  { vptr_->accelerate(on_heap_ ? ptr_ : &buffer_); }
  ///};

  {
    Printable printable{
      WaterSpell{"printableWaterSpell1", "printableWaterSpell2"}};
    printable.some_test_func_proxy("arg1");

    pass_by_cref(printable, "arg2");
  }

  // test:
  // 1 uses function that returns value
  // 2 uses both default and custom types
  {
    {
      FireSpell fs{"notpolymorphicA1", "notpolymorphicB1"};
      IntSummable ihs(std::move(fs));
      const int result = ihs.sum_with(1, 7);
      LOG(INFO)
        << "sum_with done, result: "
        << result;
    }

    std::vector<InHeapIntSummable> vec;
    FireSpell fs{"notpolymorphicA1", "notpolymorphicB1"};
    //vec.push_back(&fs); // must be disallowed
    vec.push_back(std::move(fs));
    vec.push_back(InHeapIntSummable{5});
    vec.push_back(double{0.5});
    vec.push_back(double{0.1});

    for(const InHeapIntSummable& item: vec)
    {
      const int result = item.sum_with(1, 7);
      LOG(INFO)
        << "sum_with done, result: "
        << result;
    }
  }

  {
    // Use `Typeclass<MagicItemTraits>` only for polymorphic objects.
    //
    // Code generated by typeclass can be used both with polymorphic (`Typeclass<MagicItemTraits>`) and with normal objects (`FireSpell fs`).
    //
    // i.e. for ordinary types can use methods generated by typeclass like so:

    FireSpell fs{"notpolymorphicA1", "notpolymorphicB1"};
    has_enough_mana<MagicItem::typeclass>(fs, "spellname");
  }

  {
    // `InplaceTypeclass<SpellTraits>` must be same as `InplaceSpell`
    // i.e. using InplaceSpell = InplaceTypeclass<SpellTraits>;
    InPlaceSpell myspell{
      FireSpell{"title0", "description0"}};

    myspell.set_spell_power(
      /*spellname*/ "spellname0"
      , /*spellpower*/ 0);

    myspell.cast(
      /*spellname*/ "spellname0"
      , /*spellpower*/ 0
      , /*target*/ "target0");
  }

  {
    std::vector<InPlaceSpell> spells;
    spells.push_back(FireSpell{"FireSpelltitle1"
      , "FireSpelldescription1"});
    {
      WaterSpell ws{"WaterSpelltitle1"
        , "WaterSpelldescription1"};
      spells.push_back(/*copy*/ ws);
    }
    {
      WaterSpell ws{"WaterSpelltitle2"
        , "WaterSpelldescription2"};
      spells.push_back(std::move(ws));
    }
    for(const InplaceTypeclass<SpellTraits>& spell
        : spells)
    {
      spell.cast(
        /*spellname*/ "spellname123"
        , /*spellpower*/ 123
        , /*target*/ "target123");
    }
  }

  {
    // `Typeclass<SpellTraits>` must be same as `Spell`
    // i.e. using Spell = Typeclass<SpellTraits>;
    Typeclass<SpellTraits> myspell{
      FireSpell{"title1", "description1"}};

    myspell.set_spell_power(
      /*spellname*/ "spellname1"
      , /*spellpower*/ 3);

    myspell.cast(
      /*spellname*/ "spellname1"
      , /*spellpower*/ 3
      , /*target*/ "target1");
  }

  {
    FireSpell fireSpell{"title2", "description2"};
    InplaceTypeclass<SpellTraits> myspell(fireSpell);

    InplaceTypeclass<SpellTraits> myspell_copy = myspell; // copy Spell

    myspell_copy.set_spell_power(
      /*spellname*/ "spellname2"
      , /*spellpower*/ 33);

    myspell_copy.cast(
      /*spellname*/ "spellname2"
      , /*spellpower*/ 33
      , /*target*/ "target2");
  }

  {
    FireSpell spell {"title3", "description3"};
    Spell myspell{std::move(spell)};

    Spell myspell_copy{myspell}; // copy Spell

    myspell_copy.set_spell_power(
      /*spellname*/ "spellname3"
      , /*spellpower*/ 333);

    myspell_copy.cast(
      /*spellname*/ "spellname3"
      , /*spellpower*/ 333
      , /*target*/ "target3");
  }

  {
    FireSpell spell {"title4", "description4"};
    Spell myspell = std::move(spell);

    Spell myspell_move{std::move(myspell)}; // move Spell

    myspell_move.set_spell_power(
      /*spellname*/ "spellname4"
      , /*spellpower*/ 444);

    myspell_move.cast(
      /*spellname*/ "spellname4"
      , /*spellpower*/ 444
      , /*target*/ "target4");
  }

  {
    FireSpell spell {"title5", "description5"};

    {
      Spell myspell = spell;

      Spell myspell_move = std::move(myspell); // move Spell

      myspell_move.set_spell_power(
        /*spellname*/ "spellname5"
        , /*spellpower*/ 555);

      myspell_move.cast(
        /*spellname*/ "spellname5"
        , /*spellpower*/ 555
        , /*target*/ "target5");
    }

    {
      Spell somespell{spell};

      Spell myspell{std::move(somespell)};

      Spell myspell_move = std::move(myspell); // move Spell

      myspell_move.set_spell_power(
        /*spellname*/ "spellname7"
        , /*spellpower*/ 777);

      myspell_move.cast(
        /*spellname*/ "spellname7"
        , /*spellpower*/ 777
        , /*target*/ "target7");
    }
  }

  {
    std::vector<MagicLongType> spellmagicItems;
    {
      MagicLongType pushed{
        FireSpell{"someTmpSpell0", "someTmpSpell0"}};
      spellmagicItems.push_back(std::move(pushed));
    }

    {
      MagicLongType pushed{
        FireSpell{"someTmpSpell12", "someTmpSpell12"}};
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

  {
    std::vector<MagicItem> magicItems;
    magicItems.push_back(
      FireSpell{"FireSpelltitle1", "description1"});
    magicItems.push_back(
      WaterSpell{"WaterSpelltitle1", "description1"});

    for(const MagicItem& it
        : magicItems)
    {
      it.has_enough_mana("");
    }
  }
}
