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
#include "base/files/file_path.h"
#include "base/strings/string_util.h"
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

#include "generated/Spell.typeclass.generated.hpp"
#include "generated/FireSpell_MagicItem.typeclass_instance.generated.hpp"

#include "generated/Spell.typeclass.generated.hpp"
#include "generated/WaterSpell_MagicItem.typeclass_instance.generated.hpp"

#include "generated/MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass.generated.hpp"

#include "generated/FireSpell_MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass_instance.generated.hpp"

#include "generated/MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass.generated.hpp"

#include "generated/WaterSpell_MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass_instance.generated.hpp"

#include "generated/Printable.typeclass.generated.hpp"
#include "generated/FireSpell_Printable.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void has_enough_mana<MagicItem, FireSpell>
    (const FireSpell& data, const char* spellname) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) has_enough_mana " << " by "
      << data.title << " " << spellname << std::endl;
}

} // namespace cxxctp
} // namespace generated

namespace cxxctp {
namespace generated {

template<>
void has_enough_mana<MagicItem, WaterSpell>
    (const WaterSpell& data, const char* spellname) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) has_enough_mana " << " by "
      << data.title << " " << spellname << std::endl;
}

} // namespace cxxctp
} // namespace generated

namespace cxxctp {
namespace generated {

template<>
void has_T<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , FireSpell >
(const FireSpell& data, const std::string &name1, const int &name2) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(Fire) has_T on " << name1
              << " by " << name2 << " "
              << std::endl;
}

template<>
void has_P1<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , FireSpell >
(const FireSpell& data, const char *name1) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(FireSpell) has_P1 on " << name1
              << std::endl;
}

template<>
void has_P2<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , FireSpell >
(const FireSpell& data, const int& name1) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(FireSpell) has_P2 on " << name1
              << std::endl;
}

} // namespace cxxctp
} // namespace generated

namespace cxxctp {
namespace generated {

template<>
void has_T<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , WaterSpell >
(const WaterSpell& data, const std::string &name1, const int &name2) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(WaterSpell) has_T on " << name1
              << " by " << name2 << " "
              << std::endl;
}

template<>
void has_P1<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , WaterSpell >
(const WaterSpell& data, const char *name1) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(WaterSpell) has_P1 on " << name1
              << std::endl;
}

template<>
void has_P2<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , WaterSpell >
(const WaterSpell& data, const int& name1) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(WaterSpell) has_P2 on " << name1
              << std::endl;
}

} // namespace cxxctp
} // namespace generated

namespace cxxctp {
namespace generated {

template<>
void print<Printable, FireSpell>
    (const FireSpell& data) noexcept {
    std::cout << "(lib1) print for FireSpell "
      << data.title << " " << data.description << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/Printable.typeclass.generated.hpp"
#include "generated/WaterSpell_Printable.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void print<Printable, WaterSpell>
    (const WaterSpell& data) noexcept {
    std::cout << "(lib1) print for WaterSpell "
      << data.title << " " << data.description << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/Spell.typeclass.generated.hpp"
#include "generated/FireSpell_Spell.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void cast<Spell, FireSpell>
    (const FireSpell& data, const char* spellname, const int spellpower,
     const char* target) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) cast on " << target << " by " << data.title << " " << spellname
              << " with " << spellpower << std::endl;
}

template<>
void has_spell<Spell, FireSpell>(const FireSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) has_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void add_spell<Spell, FireSpell>(const FireSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) add_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void remove_spell<Spell, FireSpell>(const FireSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) remove_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void set_spell_power<Spell, FireSpell>
    (const FireSpell& data, const char *spellname, const int spellpower) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) set_spell_power by " << data.title << " " << spellname
              << " with " << spellpower << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/Spell.typeclass.generated.hpp"
#include "generated/WaterSpell_Spell.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void cast<Spell, WaterSpell>
    (const WaterSpell& data, const char* spellname, const int spellpower,
     const char* target) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) cast on " << target << " by " << data.title << " " << spellname
              << " with " << spellpower << std::endl;
}

template<>
void has_spell<Spell, WaterSpell>(const WaterSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) has_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void add_spell<Spell, WaterSpell>(const WaterSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) add_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void remove_spell<Spell, WaterSpell>(const WaterSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) remove_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void set_spell_power<Spell, WaterSpell>
    (const WaterSpell& data, const char *spellname, const int spellpower) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) set_spell_power by " << data.title << " " << spellname
              << " with " << spellpower << std::endl;
}

} // namespace cxxctp
} // namespace generated

TEST(Typeclass, TypeclassGeneration) {
  using namespace cxxctp::generated;

  // TODO: better example https://blog.rust-lang.org/2015/05/11/traits.html

  _tc_combined_t<Spell> myspell{FireSpell{"title1", "description1"}};

  myspell.set_spell_power(/*spellname*/ "spellname1", /*spellpower*/ 3);
  myspell.cast(/*spellname*/ "spellname1", /*spellpower*/ 3, /*target*/ "target1");

  _tc_combined_t<Spell> myspellcopy = myspell;

  //_tc_impl_t<FireSpell, Spell> someFireSpell{FireSpell{"title1", "description1"}};

  //_tc_combined_t<Spell> someFireSpellmove = std::move(someFireSpell);

  //FireSpell& data = myspell.ref_model()->ref_concrete<FireSpell>();
  //FireSpell a = data;

  /*_tc_impl_t<FireSpell, Spell> myFireSpell{FireSpell{"title1", "description1"}};
  _tc_combined_t<Spell> myFireSpellref = std::ref(myFireSpell);*/

  _tc_combined_t<Spell> someFireSpell{FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}};

  std::vector<_tc_combined_t<Spell>> spells;
  spells.push_back(myspell);
  spells.push_back(someFireSpell);

  for(const _tc_combined_t<Spell>& it : spells) {
    it.cast("", 1, "");
#if defined(ENABLE_TYPECLASS_GUID)
    std::cout << "spells: get_GUID "
      << it.get_GUID() << std::endl;
#endif // ENABLE_TYPECLASS_GUID
  }

  std::vector<_tc_combined_t<MagicItem>> magicItems;
  magicItems.push_back(FireSpell{"FireSpelltitle1", "description1"});
  magicItems.push_back(WaterSpell{"WaterSpelltitle1", "description1"});

  for(const _tc_combined_t<MagicItem>& it : magicItems) {
    it.has_enough_mana("");
  }

  std::vector<_tc_combined_t<Spell, MagicItem>> spellMagicItems;
  {
    _tc_combined_t<Spell, MagicItem> pushed{};
    pushed = magicItems.at(0); // copy
    spellMagicItems.push_back(std::move(pushed));
  }
  {
    _tc_combined_t<Spell, MagicItem> pushed{};
    _tc_combined_t<Spell> someTmpSpell{
      FireSpell{"someTmpSpell", "someTmpSpell"}};
    pushed = std::move(someTmpSpell); // move
    spellMagicItems.push_back(std::move(pushed));
  }
  //spellMagicItems.push_back(someFireSpell.raw_model());
  //spellMagicItems.push_back(
  //  someFireSpell.ref_model()); // shared data
  //spellMagicItems.push_back(someFireSpell.clone_model());

  for(const _tc_combined_t<Spell, MagicItem>& it : spellMagicItems) {
    if(it.has_model<Spell>()) {
      it.cast("", 1, "");
    }
    if(it.has_model<MagicItem>()) {
      it.has_enough_mana("");
    }
  }

#if 0
  /*_tc_combined_t<Spell, MagicItem> combined1 {
      _tc_combined_t<Spell>{FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}}
  };*/

  _tc_combined_t<Spell, MagicItem> combined1 {
      FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}
  };

  if(combined1.has_model<MagicItem>()) {
    combined1.has_enough_mana("");
  }

  //combined1 = WaterSpell{"WaterSpell", "WaterSpell"};

  if(combined1.has_model<MagicItem>()) {
    combined1.has_enough_mana("");
  }

  if(combined1.has_model<Spell>()) {
    combined1.add_spell("");
  }

  combined1 = magicItems.at(0);

  if(combined1.has_model<MagicItem>()) {
    combined1.has_enough_mana("");
  }

  if(combined1.has_model<Spell>()) {
    combined1.add_spell("");
  }

  /*combined1 = _tc_combined_t<Spell>{
    FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}
  };*/

  _tc_combined_t<Spell, MagicItem> combined2 {
      FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}
  };

  std::cout << "combined2: can_convert to MagicItem: "
    << combined2.can_convert<Spell>() << std::endl;

  std::cout << "combined2: can_convert to MagicItem: "
    << combined2.can_convert<MagicItem>() << std::endl;

  std::cout << "combined2: can_convert to int: "
    << combined2.can_convert<int>() << std::endl;

  if(combined2.has_model<MagicItem>()) {
    combined1.has_enough_mana("");
  }

  if(combined2.has_model<Spell>()) {
    combined1.add_spell("");
  }

  std::vector<_tc_combined_t<Printable>> printables;
  printables.push_back(FireSpell{"someFireSpellTitle", "someFireSpelldescription1"});
  printables.push_back(WaterSpell{"WaterSpell", "WaterSpell"});

  for(const _tc_combined_t<Printable>& it : printables) {
    it.print();
  }

  std::vector<_tc_combined_t<
    MagicTemplated<std::string, int>,ParentTemplated_1<const char *>,ParentTemplated_2<const int &>
  >> tpls;
  tpls.push_back({
      WaterSpell{"WaterSpell", "WaterSpell"}
  });
  tpls.push_back({
      FireSpell{"FireSpell", "FireSpell"}
  });

  int idx = 0;
  for(const _tc_combined_t<
    MagicTemplated<std::string, int>,ParentTemplated_1<const char *>,ParentTemplated_2<const int &>
    >& it : tpls) {
    it.has_T("name1", idx++);
    it.has_P1("name~");
    it.has_P2(idx);
#if defined(ENABLE_TYPECLASS_GUID)
    std::cout << "tpls: get_GUID "
      << it.get_GUID() << std::endl;
#endif // ENABLE_TYPECLASS_GUID
  }

  /// \note Uses std::reference_wrapper
  FireSpell fs{"FireSpellRef", "FireSpellRef!"};
  _tc_combined_t<Spell, MagicItem> combinedRef1 {
      std::ref(fs)
  };

  _tc_combined_t<Spell, MagicItem> combinedRef2;
  combinedRef2.create_model<Spell>
    (std::ref(fs));

  fs.title = "NewSharedFireSpellRefTitle0";
  if(combinedRef1.has_model<Spell>()) {
    combinedRef1.cast("", 0, "");
  }
  if(combinedRef2.has_model<Spell>()) {
    combinedRef2.cast("", 0, "");
  }

  /// \note Uses std::shared_ptr
  combinedRef2.ref_model<Spell>()
    = combinedRef1.ref_model<Spell>();

  fs.title = "NewSharedFireSpellRefTitle1";
  if(combinedRef1.has_model<Spell>()) {
    combinedRef1.cast("", 0, "");
  }
  if(combinedRef2.has_model<Spell>()) {
    combinedRef2.cast("", 0, "");
  }

  /// \note Uses std::unique_ptr, data copyed!
  combinedRef2 = combinedRef1;

  fs.title = "New__NOT_SHARED__FireSpellRefTitle!!!";
  if(combinedRef1.has_model<Spell>()) {
    combinedRef1.cast("", 0, "");
  }
  if(combinedRef2.has_model<Spell>()) {
    combinedRef2.cast("", 0, "");
  }
#endif //0
}

TEST(AnnotationParser, LinksWithClingLLVM) {
  ::flexlib::AnnotationMethods annotationMethods;
  ::clang_utils::SourceTransformPipeline sourceTransformPipeline;

  ::flexlib::AnnotationParser annotationParser(&annotationMethods);

  ::flexlib::AnnotationMatchHandler anotationMatchHandler(
      &annotationParser
      , &annotationMethods
      // SaveFileHandler
      , base::BindRepeating(
          [
          ](
            const clang::FileID& fileID
            , const clang::FileEntry* fileEntry
            , clang::Rewriter& rewriter
          ){

          }
      ));

  scoped_refptr<clang_utils::AnnotationMatchOptions>
    annotationMatchOptions
      = new clang_utils::AnnotationMatchOptions(
          ::flexlib::kAnnotateAttrName
          , base::BindRepeating(
              &::flexlib::AnnotationMatchHandler::matchHandler
              , base::Unretained(&anotationMatchHandler))
          , base::BindRepeating(
              &::flexlib::AnnotationMatchHandler::endSourceFileHandler
              , base::Unretained(&anotationMatchHandler))
        );

  // see http://llvm.org/docs/doxygen/html/classllvm_1_1cl_1_1OptionCategory.html
  llvm::cl::OptionCategory UseOverrideCategory("Use override options");

  // Dummy test.
  // We just check that code can compile and link with LLVM cling libs
  EXPECT_TRUE(annotationMethods.empty());
}
