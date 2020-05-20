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
#include <base/third_party/dynamic_annotations/dynamic_annotations.h>
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

#include "Spell.typeclass.generated.hpp"
#include "FireSpell_MagicItem.typeclass_instance.generated.hpp"

#include "Spell.typeclass.generated.hpp"
#include "WaterSpell_MagicItem.typeclass_instance.generated.hpp"

#include "MagicLongType.typeclass.generated.hpp"

#include "FireSpell_MagicLongType.typeclass_instance.generated.hpp"

#include "WaterSpell_MagicLongType.typeclass_instance.generated.hpp"

#include "Printable.typeclass.generated.hpp"
#include "FireSpell_Printable.typeclass_instance.generated.hpp"

namespace poly {
namespace generated {

// allow FireSpell to be used as MagicItemTraits
// MagicItemTraits is base class (typeclass)
template<>
void has_enough_mana<MagicItem::typeclass>
  (const FireSpell& data, const char* spellname) noexcept
{
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
    << "(lib1) has_enough_mana " << " by "
    << data.title << " " << spellname;
}

} // namespace poly
} // namespace generated

namespace poly {
namespace generated {

// allow WaterSpell to be used as MagicItemTraits
// MagicItemTraits is base class (typeclass)
template<>
void has_enough_mana<Typeclass<DEFINE_MagicItem>>
  (const WaterSpell& data, const char* spellname) noexcept
{
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
    << "(lib1) has_enough_mana " << " by "
    << data.title << " " << spellname;
}

} // namespace poly
} // namespace generated

namespace poly {
namespace generated {

template<>
void has_T<
    MagicLongType
  >(const FireSpell& data
  , const std::string &name1
  , const int &name2) noexcept
{
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
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
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
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
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
    << "(FireSpell) has_P2 on " << name1
    << " by "
    << data.title
    << " ";
}

} // namespace poly
} // namespace generated

namespace poly {
namespace generated {

template<>
void has_T<
    MagicLongType
  >(const WaterSpell& data
  , const std::string &name1
  , const int &name2) noexcept
{
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
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
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
    << "(WaterSpell) has_P1 on " << name1
    << " by "
    << data.title
    << " ";
}

template<>
void has_P2<
    MagicLongType
  >(const WaterSpell& data, const int& name1) noexcept
{
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
    << "(WaterSpell) has_P2 on " << name1;
}

} // namespace poly
} // namespace generated

namespace poly {
namespace generated {

template<>
void print<Printable::typeclass>
  (const FireSpell& data) noexcept
{
  LOG(WARNING)
    << "(lib1) print for FireSpell "
    << data.title << " " << data.description;
}

} // namespace poly
} // namespace generated

#include "Printable.typeclass.generated.hpp"
#include "WaterSpell_Printable.typeclass_instance.generated.hpp"

namespace poly {
namespace generated {

template<>
void print<Printable>
  (const WaterSpell& data) noexcept
{
  LOG(WARNING)
    << "(lib1) print for WaterSpell "
    << data.title << " " << data.description;
}

} // namespace poly
} // namespace generated

#include "Spell.typeclass.generated.hpp"
#include "FireSpell_Spell.typeclass_instance.generated.hpp"

namespace poly {
namespace generated {

template<>
void cast<Spell::typeclass>
  (const FireSpell& data, const char* spellname, const int spellpower,
   const char* target) noexcept
{
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
    << "(lib1) cast on " << target
    << " by " << data.title << " " << spellname
    << " with " << spellpower;
}

template<>
void has_spell<Spell>(
  const FireSpell& data, const char *spellname ) noexcept
{
  LOG(WARNING)
    << "(lib1) has_spell by "
    << data.title << " " << spellname
    << " with " << spellname;
}

template<>
void add_spell<Spell::typeclass>(
  const FireSpell& data
  , const char *spellname ) noexcept
{
  LOG(WARNING)
    << "(lib1) add_spell by " << data.title << " " << spellname
    << " with " << spellname;
}

template<>
void remove_spell<Spell::typeclass>(
  const FireSpell& data
  , const char *spellname ) noexcept
{
  LOG(WARNING)
    << "(lib1) remove_spell by " << data.title << " " << spellname
    << " with " << spellname;
}

template<>
void set_spell_power<Spell::typeclass>
    (const FireSpell& data
    , const char *spellname
    , const int spellpower) noexcept
{
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
    << "(lib1) set_spell_power by " << data.title << " " << spellname
    << " with " << spellpower;
}

} // namespace poly
} // namespace generated

#include "Spell.typeclass.generated.hpp"
#include "WaterSpell_Spell.typeclass_instance.generated.hpp"

void some_test_func
  (WaterSpell& data, const char* arg1) noexcept;

namespace poly {
namespace generated {

template<>
void cast<Spell::typeclass>
  (const WaterSpell& data
  , const char* spellname
  , const int spellpower,
   const char* target) noexcept
{
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
    << "(lib1) cast on " << target
    << " by " << data.title << " " << spellname
    << " with " << spellpower;
}

template<>
void has_spell<Spell::typeclass>(
  const WaterSpell& data, const char *spellname ) noexcept
{
  LOG(WARNING)
    << "(lib1) has_spell by "
    << data.title << " " << spellname
    << " with " << spellname;
}

template<>
void add_spell<Spell::typeclass>(
  const WaterSpell& data, const char *spellname ) noexcept
{
  LOG(WARNING)
    << "(lib1) add_spell by " << data.title << " " << spellname
    << " with " << spellname;
}

template<>
void remove_spell<Spell>(
  const WaterSpell& data, const char *spellname ) noexcept
{
  LOG(WARNING)
    << "(lib1) remove_spell by " << data.title << " " << spellname
    << " with " << spellname;
}

template<>
void set_spell_power<Spell::typeclass>
  (const WaterSpell& data
  , const char *spellname
  , const int spellpower) noexcept
{
  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
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
//  LOG(WARNING)
//    << "(lib1) print for FireSpell "
//    << data.title << " " << data.description;
//}

} // namespace poly
} // namespace generated

void some_test_func
  (WaterSpell& data, const char* arg1) noexcept
{
  data.title = "changed title";

  /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
  LOG(WARNING)
    << "some_test_func "
    << " by "
    << data.title
    << " "
    << arg1;
}

TEST(Typeclass, TypeclassGeneration) {
  using namespace poly::generated;

  {
    Printable printable{
      WaterSpell{"printableWaterSpell1", "printableWaterSpell2"}};
    printable.some_test_func_proxy("arg1");
  }

  /// \note not init-ed will crash
  //{
  //  InplaceTypeclass<SpellTraits> myspell;
  //  myspell.set_spell_power(
  //    /*spellname*/ "spellname4"
  //    , /*spellpower*/ 444);
  //}

  {
    // Use `Typeclass<MagicItemTraits>` only for polymorphic objects.
    //
    // Code generated by typeclass can be used both with polymorphic (`Typeclass<MagicItemTraits>`) and with normal objects (`FireSpell fs`).
    //
    // i.e. for ordinary types can use methods generated by typeclass like so:

    FireSpell fs{"notPolymorphicA1", "notPolymorphicB1"};
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
    InplaceTypeclass<DEFINE_Spell> myspell = std::move(spell);

    InPlaceSpell myspell_move{std::move(myspell)}; // move Spell

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
      Typeclass<DEFINE_MagicLongType> pushed{};
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
    std::vector<InplaceTypeclass<DEFINE_MagicItem>> magicItems;
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
