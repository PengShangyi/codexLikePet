qt_add_executable(potato_smoke_test
    test_smoke.cpp
)

target_link_libraries(potato_smoke_test PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME potato_smoke_test COMMAND potato_smoke_test)

qt_add_executable(potato_single_instance_test
    test_single_instance.cpp
    ../src/app/SingleInstance.cpp
    ../src/app/SingleInstance.h
)
target_include_directories(potato_single_instance_test PRIVATE ../src)
target_link_libraries(potato_single_instance_test PRIVATE Qt6::Core Qt6::Network Qt6::Test)
add_test(NAME potato_single_instance_test COMMAND potato_single_instance_test)

qt_add_executable(potato_window_placement_test
    test_window_placement.cpp
    ../src/pet/WindowPlacement.cpp
    ../src/pet/WindowPlacement.h
)
target_include_directories(potato_window_placement_test PRIVATE ../src)
target_link_libraries(potato_window_placement_test PRIVATE Qt6::Core Qt6::Test)
add_test(NAME potato_window_placement_test COMMAND potato_window_placement_test)

qt_add_executable(potato_atlas_test
    test_atlas.cpp
    ../src/pet/AnimationClip.cpp
    ../src/pet/AnimationClip.h
    ../src/pet/AnimationPlayer.cpp
    ../src/pet/AnimationPlayer.h
    ../src/pet/PetAtlas.cpp
    ../src/pet/PetAtlas.h
    ../src/pet/AtlasCache.cpp
    ../src/pet/AtlasCache.h
)
target_include_directories(potato_atlas_test PRIVATE ../src)
target_link_libraries(potato_atlas_test PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME potato_atlas_test COMMAND potato_atlas_test)

qt_add_executable(potato_package_validator_test
    test_package_validator.cpp
    ../src/pet/PetAtlas.cpp
    ../src/pet/PetAtlas.h
    ../src/resources/PetPackage.cpp
    ../src/resources/PetPackage.h
    ../src/resources/PetPackageValidator.cpp
    ../src/resources/PetPackageValidator.h
)
target_include_directories(potato_package_validator_test PRIVATE ../src)
target_link_libraries(potato_package_validator_test PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME potato_package_validator_test COMMAND potato_package_validator_test)

qt_add_executable(potato_builtin_pet_test
    test_builtin_pet.cpp
    ../src/pet/PetAtlas.cpp
    ../src/pet/PetAtlas.h
    ../src/resources/PetPackage.cpp
    ../src/resources/PetPackage.h
    ../src/resources/PetPackageValidator.cpp
    ../src/resources/PetPackageValidator.h
)
target_include_directories(potato_builtin_pet_test PRIVATE ../src)
target_compile_definitions(potato_builtin_pet_test PRIVATE
    POTATO_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
target_link_libraries(potato_builtin_pet_test PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME potato_builtin_pet_test COMMAND potato_builtin_pet_test)

qt_add_executable(potato_package_importer_test
    test_package_importer.cpp
    ../src/pet/PetAtlas.cpp
    ../src/pet/PetAtlas.h
    ../src/resources/PetPackage.cpp
    ../src/resources/PetPackage.h
    ../src/resources/PetPackageValidator.cpp
    ../src/resources/PetPackageValidator.h
    ../src/resources/ArchiveExtractor.cpp
    ../src/resources/ArchiveExtractor.h
    ../src/resources/PetPackageImporter.cpp
    ../src/resources/PetPackageImporter.h
    ../src/resources/PetStore.cpp
    ../src/resources/PetStore.h
)
target_include_directories(potato_package_importer_test PRIVATE ../src)
target_link_libraries(potato_package_importer_test PRIVATE Qt6::Core Qt6::Gui Qt6::Test miniz)
add_test(NAME potato_package_importer_test COMMAND potato_package_importer_test)

qt_add_executable(potato_settings_test
    test_settings.cpp
    ../src/settings/AppSettings.cpp
    ../src/settings/AppSettings.h
    ../src/settings/Localization.cpp
    ../src/settings/Localization.h
)
target_include_directories(potato_settings_test PRIVATE ../src)
target_link_libraries(potato_settings_test PRIVATE Qt6::Core Qt6::Test)
add_test(NAME potato_settings_test COMMAND potato_settings_test)

qt_add_executable(potato_settings_window_test
    test_settings_window.cpp
    ../src/settings/AppSettings.cpp
    ../src/settings/AppSettings.h
    ../src/settings/Localization.cpp
    ../src/settings/Localization.h
    ../src/settings/SettingsWindow.cpp
    ../src/settings/SettingsWindow.h
    ../src/settings/PetPreviewWidget.cpp
    ../src/settings/PetPreviewWidget.h
    ../src/pet/AnimationClip.cpp
    ../src/pet/AnimationClip.h
    ../src/pet/PetAtlas.cpp
    ../src/pet/PetAtlas.h
)
target_include_directories(potato_settings_window_test PRIVATE ../src)
target_link_libraries(potato_settings_window_test PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME potato_settings_window_test COMMAND potato_settings_window_test)
set_tests_properties(potato_settings_window_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_add_executable(potato_library_test
    test_library.cpp
    ../src/pet/PetAtlas.cpp
    ../src/pet/PetAtlas.h
    ../src/resources/PetPackage.cpp
    ../src/resources/PetPackage.h
    ../src/resources/PetPackageValidator.cpp
    ../src/resources/PetPackageValidator.h
    ../src/resources/PetStore.cpp
    ../src/resources/PetStore.h
    ../src/resources/PetLibrary.cpp
    ../src/resources/PetLibrary.h
)
target_include_directories(potato_library_test PRIVATE ../src)
target_link_libraries(potato_library_test PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME potato_library_test COMMAND potato_library_test)

qt_add_executable(potato_behavior_test
    test_behavior.cpp
    ../src/pet/BehaviorController.cpp
    ../src/pet/BehaviorController.h
    ../src/pet/WindowPlacement.cpp
    ../src/pet/WindowPlacement.h
    ../src/quotes/SpeechBubble.cpp
    ../src/quotes/SpeechBubble.h
    ../src/quotes/QuoteProvider.cpp
    ../src/quotes/QuoteProvider.h
    ../src/platform/WindowOverlay.cpp
    ../src/platform/WindowOverlay.h
    ../src/platform/MacWindowOverlay.mm
)
target_include_directories(potato_behavior_test PRIVATE ../src)
target_link_libraries(potato_behavior_test PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test "-framework AppKit")
add_test(NAME potato_behavior_test COMMAND potato_behavior_test)

qt_add_executable(potato_window_overlay_test
    test_window_overlay.cpp
    ../src/platform/WindowOverlay.cpp
    ../src/platform/WindowOverlay.h
)
target_include_directories(potato_window_overlay_test PRIVATE ../src)
target_link_libraries(potato_window_overlay_test PRIVATE Qt6::Core Qt6::Test)
add_test(NAME potato_window_overlay_test COMMAND potato_window_overlay_test)

# Exercises the real NSWindow overlay state end to end. Deliberately NOT run under
# offscreen: it self-skips (QSKIP) unless the cocoa QPA platform is active.
qt_add_executable(potato_pet_overlay_test
    test_pet_overlay.mm
    ../src/pet/PetWindow.cpp
    ../src/pet/PetWindow.h
    ../src/pet/PetGesture.cpp
    ../src/pet/PetGesture.h
    ../src/pet/WindowPlacement.cpp
    ../src/pet/WindowPlacement.h
    ../src/platform/WindowOverlay.cpp
    ../src/platform/WindowOverlay.h
    ../src/platform/MacWindowOverlay.mm
    ../src/settings/AppSettings.cpp
    ../src/settings/AppSettings.h
)
target_include_directories(potato_pet_overlay_test PRIVATE ../src)
target_link_libraries(potato_pet_overlay_test PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test "-framework AppKit")
add_test(NAME potato_pet_overlay_test COMMAND potato_pet_overlay_test)

qt_add_executable(potato_typing_activity_test
    test_typing_activity.cpp
    ../src/input/TypingActivityDetector.cpp
    ../src/input/TypingActivityDetector.h
    ../src/input/InputActivitySource.h
)
target_include_directories(potato_typing_activity_test PRIVATE ../src)
target_link_libraries(potato_typing_activity_test PRIVATE Qt6::Core Qt6::Test)
add_test(NAME potato_typing_activity_test COMMAND potato_typing_activity_test)

qt_add_executable(potato_environment_test
    test_environment.cpp
    ../src/environment/EnvironmentClock.cpp
    ../src/environment/EnvironmentClock.h
    ../src/environment/EnvironmentResolver.cpp
    ../src/environment/EnvironmentResolver.h
    ../src/environment/EnvironmentState.cpp
    ../src/environment/EnvironmentState.h
    ../src/settings/AppSettings.cpp
    ../src/settings/AppSettings.h
    ../src/resources/PetPackage.cpp
    ../src/resources/PetPackage.h
)
target_include_directories(potato_environment_test PRIVATE ../src)
target_link_libraries(potato_environment_test PRIVATE Qt6::Core Qt6::Test)
add_test(NAME potato_environment_test COMMAND potato_environment_test)

qt_add_executable(potato_motion_test
    test_motion.cpp
    ../src/accessibility/MotionController.cpp
    ../src/accessibility/MotionController.h
    ../src/platform/SystemActivitySource.h
    ../src/settings/AppSettings.cpp
    ../src/settings/AppSettings.h
)
target_include_directories(potato_motion_test PRIVATE ../src)
target_link_libraries(potato_motion_test PRIVATE Qt6::Core Qt6::Test)
add_test(NAME potato_motion_test COMMAND potato_motion_test)

qt_add_executable(potato_login_item_test
    test_login_item.cpp
    ../src/login/LoginItemCoordinator.cpp
    ../src/login/LoginItemCoordinator.h
    ../src/login/LoginItemController.h
    ../src/settings/AppSettings.cpp
    ../src/settings/AppSettings.h
)
target_include_directories(potato_login_item_test PRIVATE ../src)
target_link_libraries(potato_login_item_test PRIVATE Qt6::Core Qt6::Test)
add_test(NAME potato_login_item_test COMMAND potato_login_item_test)

qt_add_executable(potato_pet_gesture_test
    test_pet_gesture.cpp
    ../src/pet/PetGesture.cpp
    ../src/pet/PetGesture.h
    ../src/pet/WindowPlacement.cpp
    ../src/pet/WindowPlacement.h
)
target_include_directories(potato_pet_gesture_test PRIVATE ../src)
target_link_libraries(potato_pet_gesture_test PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME potato_pet_gesture_test COMMAND potato_pet_gesture_test)

qt_add_executable(potato_idle_scheduler_test
    test_idle_scheduler.cpp
    ../src/pet/IdleActivityScheduler.cpp
    ../src/pet/IdleActivityScheduler.h
    ../src/pet/PetAtlas.cpp
    ../src/pet/PetAtlas.h
)
target_include_directories(potato_idle_scheduler_test PRIVATE ../src)
target_link_libraries(potato_idle_scheduler_test PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME potato_idle_scheduler_test COMMAND potato_idle_scheduler_test)

# AppController is the wiring hub, so its test links most of the app. Fakes are
# injected via AppController::Dependencies; the .mm adapters are linked but their
# platform code never runs under RuntimeCheck. Runs offscreen for CI.
qt_add_executable(potato_app_controller_test
    test_app_controller.cpp
    ../src/app/AppController.cpp
    ../src/app/AppController.h
    ../src/app/AppNotifier.cpp
    ../src/app/AppNotifier.h
    ../src/settings/AppSettings.cpp
    ../src/settings/AppSettings.h
    ../src/settings/Localization.cpp
    ../src/settings/Localization.h
    ../src/settings/SettingsWindow.cpp
    ../src/settings/SettingsWindow.h
    ../src/settings/OnboardingWindow.cpp
    ../src/settings/OnboardingWindow.h
    ../src/settings/AboutWindow.cpp
    ../src/settings/AboutWindow.h
    ../src/settings/PetPreviewWidget.cpp
    ../src/settings/PetPreviewWidget.h
    ../src/pet/PetWindow.cpp
    ../src/pet/PetWindow.h
    ../src/pet/AnimationPlayer.cpp
    ../src/pet/AnimationPlayer.h
    ../src/pet/AnimationClip.cpp
    ../src/pet/AnimationClip.h
    ../src/pet/BehaviorController.cpp
    ../src/pet/BehaviorController.h
    ../src/pet/IdleActivityScheduler.cpp
    ../src/pet/IdleActivityScheduler.h
    ../src/pet/AtlasCache.cpp
    ../src/pet/AtlasCache.h
    ../src/pet/PetAtlas.cpp
    ../src/pet/PetAtlas.h
    ../src/pet/WindowPlacement.cpp
    ../src/pet/WindowPlacement.h
    ../src/pet/PetGesture.cpp
    ../src/pet/PetGesture.h
    ../src/resources/PetPackage.cpp
    ../src/resources/PetPackage.h
    ../src/resources/PetPackageValidator.cpp
    ../src/resources/PetPackageValidator.h
    ../src/resources/ArchiveExtractor.cpp
    ../src/resources/ArchiveExtractor.h
    ../src/resources/PetPackageImporter.cpp
    ../src/resources/PetPackageImporter.h
    ../src/resources/PetStore.cpp
    ../src/resources/PetStore.h
    ../src/resources/PetLibrary.cpp
    ../src/resources/PetLibrary.h
    ../src/quotes/QuoteProvider.cpp
    ../src/quotes/QuoteProvider.h
    ../src/quotes/SpeechBubble.cpp
    ../src/quotes/SpeechBubble.h
    ../src/input/InputActivitySource.h
    ../src/input/MacInputActivitySource.mm
    ../src/input/MacInputActivitySource.h
    ../src/input/TypingActivityDetector.cpp
    ../src/input/TypingActivityDetector.h
    ../src/environment/EnvironmentClock.cpp
    ../src/environment/EnvironmentClock.h
    ../src/environment/EnvironmentResolver.cpp
    ../src/environment/EnvironmentResolver.h
    ../src/environment/EnvironmentState.cpp
    ../src/environment/EnvironmentState.h
    ../src/accessibility/MotionController.cpp
    ../src/accessibility/MotionController.h
    ../src/platform/SystemActivitySource.h
    ../src/platform/MacSystemActivitySource.mm
    ../src/platform/MacSystemActivitySource.h
    ../src/platform/MacApplication.mm
    ../src/platform/MacApplication.h
    ../src/platform/WindowOverlay.cpp
    ../src/platform/WindowOverlay.h
    ../src/platform/MacWindowOverlay.mm
    ../src/login/LoginItemController.h
    ../src/login/LoginItemCoordinator.cpp
    ../src/login/LoginItemCoordinator.h
    ../src/login/MacLoginItemController.mm
    ../src/login/MacLoginItemController.h
)
target_include_directories(potato_app_controller_test PRIVATE ../src)
target_compile_definitions(potato_app_controller_test PRIVATE
    POTATO_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
target_link_libraries(potato_app_controller_test PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test miniz
    "-framework AppKit"
    "-framework ApplicationServices"
    "-framework ServiceManagement"
)
add_test(NAME potato_app_controller_test COMMAND potato_app_controller_test)
set_tests_properties(potato_app_controller_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
