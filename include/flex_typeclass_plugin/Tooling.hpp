#pragma once

#include "flex_typeclass_plugin/CodeGenerator.hpp"

#include <flexlib/clangUtils.hpp>
#include <flexlib/ToolPlugin.hpp>
#if defined(CLING_IS_ON)
#include "flexlib/ClingInterpreterModule.hpp"
#endif // CLING_IS_ON

#include <base/logging.h>
#include <base/sequenced_task_runner.h>
#include <base/files/file_path.h>

namespace flex_typeclass_plugin {

// Declaration must match plugin version.
struct Settings {
  // output directory for generated files
  std::string outDir;
};

} // namespace flex_typeclass_plugin

namespace plugin {

/// \note class name must not collide with
/// class names from other loaded plugins
class TypeclassTooling {
public:
  TypeclassTooling(
    const ::plugin::ToolPlugin::Events::RegisterAnnotationMethods& event
#if defined(CLING_IS_ON)
    , ::cling_utils::ClingInterpreter* clingInterpreter
#endif // CLING_IS_ON
  );

  ~TypeclassTooling();

  /**
   * EXAMPLE:
      // like `trait`
      $typeclass(public MagicItem)

      // like `trait`
      /// \note example of merged typeclasses
      /// \note in most cases prefer combined typeclasses to merged
      $typeclass(
          public MagicTemplated<std::string, int>
          , public ParentTemplated_1<const char *>
          , public ParentTemplated_2<const int &>)
   */
  clang_utils::SourceTransformResult
    typeclass(
      const clang_utils::SourceTransformOptions& sourceTransformOptions);

  /**
   * EXAMPLE:
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
   */
  clang_utils::SourceTransformResult
    typeclass_instance(
      const clang_utils::SourceTransformOptions& sourceTransformOptions);

  /**
   * EXAMPLE:
      // just wraps multiple `traits`, forwards calls
      /// \note example of combined typeclasses
      $typeclass_combo(
        typeclass_combo(Spell, MagicItem)
      )
   */
  //clang_utils::SourceTransformResult
  //  typeclass_combo(
  //    const clang_utils::SourceTransformOptions& sourceTransformOptions);

private:
  ::clang_utils::SourceTransformRules* sourceTransformRules_;

#if defined(CLING_IS_ON)
  ::cling_utils::ClingInterpreter* clingInterpreter_;
#endif // CLING_IS_ON

  SEQUENCE_CHECKER(sequence_checker_);

  // output directory for generated files
  base::FilePath outDir_;

  base::FilePath dir_exe_;

  TypeclassCodeGenerator typeclassCodeGenerator_{};

  flex_typeclass_plugin::Settings settings_{};

  DISALLOW_COPY_AND_ASSIGN(TypeclassTooling);
};

} // namespace plugin
