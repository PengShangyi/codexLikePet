# Test binaries are grouped by link set; CTest entries are not.
#
# Two dozen executables each linked the same Qt frameworks to run a handful of
# assertions, which cost a link and an AUTOMOC pass apiece. The classes that
# share a link set now share a binary -- but every class keeps its own CTest
# entry, dispatched with `--class` (see tests/support/TestRunner.h). So
# `ctest -R potato_settings_test` still selects exactly that class, `ctest -j`
# still has one unit of parallelism per class, and a failure still names the one
# test that broke.
#
# Four binaries stay on their own, each for a reason: potato_builtin_pet_test is
# decode-bound against the real assets, potato_pet_overlay_test must NOT run
# offscreen (it self-skips unless the cocoa platform is active),
# potato_behavior_test links the AppKit overlay, and potato_app_controller_test
# links most of the app.

# Registers one CTest entry per class in a grouped binary. CLASSES takes
# `ctest_name ClassName` pairs.
function(potato_add_grouped_tests target)
    cmake_parse_arguments(ARG "OFFSCREEN" "" "CLASSES" ${ARGN})
    list(LENGTH ARG_CLASSES count)
    math(EXPR last "${count} / 2 - 1")
    foreach(index RANGE ${last})
        math(EXPR name_at "${index} * 2")
        math(EXPR class_at "${index} * 2 + 1")
        list(GET ARG_CLASSES ${name_at} test_name)
        list(GET ARG_CLASSES ${class_at} class_name)
        add_test(NAME ${test_name} COMMAND ${target} --class ${class_name})
        if(ARG_OFFSCREEN)
            set_tests_properties(${test_name} PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        endif()
    endforeach()
endfunction()

# ---------------------------------------------------------------- grouped ---

# Everything needing no more than a QCoreApplication.
qt_add_executable(potato_core_test
    groups/core_main.cpp
    support/TestRunner.cpp
    support/TestRunner.h
    test_smoke.cpp
    test_single_instance.cpp
    test_settings.cpp
    test_typing_activity.cpp
    test_motion.cpp
    test_login_item.cpp
    test_window_overlay.cpp
    test_environment.cpp
    test_resource_summary.cpp
    ../src/app/SingleInstance.cpp
    ../src/app/SingleInstance.h
    ../src/input/InputActivitySource.h
    ../src/input/TypingActivityDetector.cpp
    ../src/input/TypingActivityDetector.h
    ../src/accessibility/MotionController.cpp
    ../src/accessibility/MotionController.h
    ../src/platform/SystemActivitySource.h
    ../src/login/LoginItemController.h
    ../src/login/LoginItemCoordinator.cpp
    ../src/login/LoginItemCoordinator.h
    ../src/resources/PetResourceSummary.cpp
    ../src/resources/PetResourceSummary.h
)
target_include_directories(potato_core_test PRIVATE ../src .)
target_link_libraries(potato_core_test PRIVATE
    potato_settings_core potato_environment potato_package potato_overlay
    Qt6::Core Qt6::Network Qt6::Test)
potato_add_grouped_tests(potato_core_test CLASSES
    potato_smoke_test            SmokeTest
    potato_single_instance_test  SingleInstanceTest
    potato_settings_test         SettingsTest
    potato_typing_activity_test  TypingActivityTest
    potato_motion_test           MotionTest
    potato_login_item_test       LoginItemTest
    potato_window_overlay_test   WindowOverlayTest
    potato_environment_test      EnvironmentTest
    potato_resource_summary_test ResourceSummaryTest)

qt_add_executable(potato_geometry_test
    groups/geometry_main.cpp
    support/TestRunner.cpp
    support/TestRunner.h
    test_window_placement.cpp
    test_pet_gesture.cpp
)
target_include_directories(potato_geometry_test PRIVATE ../src .)
target_link_libraries(potato_geometry_test PRIVATE potato_geometry
    Qt6::Core Qt6::Gui Qt6::Test)
potato_add_grouped_tests(potato_geometry_test CLASSES
    potato_window_placement_test WindowPlacementTest
    potato_pet_gesture_test      PetGestureTest)

qt_add_executable(potato_ui_test
    groups/ui_main.cpp
    support/TestRunner.cpp
    support/TestRunner.h
    test_theme.cpp
    test_ui_kit.cpp
    test_settings_window.cpp
    test_pet_guide_window.cpp
    test_pet_window.cpp
    ../src/pet/PetWindow.cpp
    ../src/pet/PetWindow.h
    ../src/platform/MacWindowOverlay.mm
    ../src/settings/SettingsWindow.cpp
    ../src/settings/SettingsWindow.h
    # Compiled directly rather than linked: the guide needs only Localization, the
    # theme and Widgets, and potato_app is off limits to a grouped test.
    ../src/settings/PetGuideWindow.cpp
    ../src/settings/PetGuideWindow.h
    ../src/settings/PetPreviewWidget.cpp
    ../src/settings/PetPreviewWidget.h
    ../src/platform/SystemSymbols.h
    ../src/platform/MacSystemSymbols.mm
)
target_include_directories(potato_ui_test PRIVATE ../src .)
target_link_libraries(potato_ui_test PRIVATE
    potato_settings_core potato_atlas potato_ui potato_theme potato_geometry potato_overlay
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test "-framework AppKit")
potato_add_grouped_tests(potato_ui_test OFFSCREEN CLASSES
    potato_theme_test           ThemeTest
    potato_ui_kit_test          UiKitTest
    potato_settings_window_test SettingsWindowTest
    potato_pet_guide_window_test PetGuideWindowTest
    potato_pet_window_test      PetWindowRenderTest)

qt_add_executable(potato_package_test
    groups/package_main.cpp
    support/TestRunner.cpp
    support/TestRunner.h
    support/AtlasFixture.cpp
    support/AtlasFixture.h
    test_package_policy.cpp
    test_package_validator.cpp
    test_library.cpp
    test_package_importer.cpp
    test_atlas_composer.cpp
    test_pet_package_writer.cpp
    ../src/resources/PetLibrary.cpp
    ../src/resources/PetLibrary.h
    ../src/resources/ArchiveExtractor.cpp
    ../src/resources/ArchiveExtractor.h
    ../src/resources/PetPackageImporter.cpp
    ../src/resources/PetPackageImporter.h
)
target_include_directories(potato_package_test PRIVATE ../src .)
target_link_libraries(potato_package_test PRIVATE potato_atlas potato_package potato_policy
    Qt6::Core Qt6::Gui Qt6::Test miniz)
potato_add_grouped_tests(potato_package_test CLASSES
    potato_package_policy_test    PackagePolicyTest
    potato_package_validator_test PackageValidatorTest
    potato_library_test           LibraryTest
    potato_package_importer_test  PackageImporterTest
    potato_atlas_composer_test    AtlasComposerTest
    potato_package_writer_test    PetPackageWriterTest)

qt_add_executable(potato_animation_test
    groups/animation_main.cpp
    support/TestRunner.cpp
    support/TestRunner.h
    support/AtlasFixture.cpp
    support/AtlasFixture.h
    test_atlas.cpp
    test_idle_scheduler.cpp
    ../src/pet/AnimationPlayer.cpp
    ../src/pet/AnimationPlayer.h
    ../src/pet/AtlasCache.cpp
    ../src/pet/AtlasCache.h
    ../src/pet/ClipCache.cpp
    ../src/pet/ClipCache.h
    ../src/pet/TypingAnimationDriver.cpp
    ../src/pet/TypingAnimationDriver.h
    ../src/pet/IdleActivityScheduler.cpp
    ../src/pet/IdleActivityScheduler.h
)
target_include_directories(potato_animation_test PRIVATE ../src .)
target_link_libraries(potato_animation_test PRIVATE potato_atlas
    Qt6::Core Qt6::Gui Qt6::Test)
potato_add_grouped_tests(potato_animation_test CLASSES
    potato_atlas_test          AtlasTest
    potato_idle_scheduler_test IdleSchedulerTest)

# --------------------------------------------------------------- solitary ---

# Validates the shipped assets rather than a synthetic fixture, so it is
# decode-bound and stays on its own to keep `ctest -j` spreading it.
qt_add_executable(potato_builtin_pet_test
    test_builtin_pet.cpp
)
target_include_directories(potato_builtin_pet_test PRIVATE ../src)
target_compile_definitions(potato_builtin_pet_test PRIVATE
    POTATO_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
target_link_libraries(potato_builtin_pet_test PRIVATE potato_atlas potato_package potato_policy
    Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME potato_builtin_pet_test COMMAND potato_builtin_pet_test)

qt_add_executable(potato_behavior_test
    test_behavior.cpp
    ../src/pet/BehaviorController.cpp
    ../src/pet/BehaviorController.h
    ../src/quotes/SpeechBubble.cpp
    ../src/quotes/SpeechBubble.h
    ../src/quotes/QuoteProvider.cpp
    ../src/quotes/QuoteProvider.h
    ../src/platform/MacWindowOverlay.mm
)
target_include_directories(potato_behavior_test PRIVATE ../src)
target_link_libraries(potato_behavior_test PRIVATE potato_geometry potato_overlay potato_theme
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test "-framework AppKit")
add_test(NAME potato_behavior_test COMMAND potato_behavior_test)

# Exercises the real NSWindow overlay state end to end. Deliberately NOT run under
# offscreen: it self-skips (QSKIP) unless the cocoa QPA platform is active, which
# is also why it cannot join the offscreen UI group.
qt_add_executable(potato_pet_overlay_test
    test_pet_overlay.mm
    ../src/pet/PetWindow.cpp
    ../src/pet/PetWindow.h
    ../src/platform/MacWindowOverlay.mm
)
target_include_directories(potato_pet_overlay_test PRIVATE ../src)
target_link_libraries(potato_pet_overlay_test PRIVATE
    potato_geometry potato_overlay potato_settings_core potato_theme
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test "-framework AppKit")
add_test(NAME potato_pet_overlay_test COMMAND potato_pet_overlay_test)

# AppController is the wiring hub, so its test needs most of the app. It links
# potato_app -- the one library that includes the macOS adapters -- because it
# and the app itself were otherwise compiling the same thirty sources twice.
# Fakes are injected via AppController::Dependencies, so the .mm adapters are
# linked but their platform code never runs under RuntimeCheck. Offscreen for CI.
qt_add_executable(potato_app_controller_test
    test_app_controller.cpp
    support/AtlasFixture.cpp
    support/AtlasFixture.h
)
target_include_directories(potato_app_controller_test PRIVATE ../src .)
target_compile_definitions(potato_app_controller_test PRIVATE
    POTATO_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
target_link_libraries(potato_app_controller_test PRIVATE
    potato_app
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME potato_app_controller_test COMMAND potato_app_controller_test)
set_tests_properties(potato_app_controller_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
