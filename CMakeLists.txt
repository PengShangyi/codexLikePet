cmake_minimum_required(VERSION 3.24)

project(Potato VERSION 0.1.0 LANGUAGES CXX OBJCXX)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(INSTALL_PROJECT OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 77d0dce8627735138c51770d1799a1ef48f2117d
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniz)
set_property(TARGET miniz PROPERTY SYSTEM TRUE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "Potato only supports Apple Silicon" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "Minimum supported macOS version" FORCE)

if(DEFINED ENV{POTATO_QT_ROOT} AND NOT "$ENV{POTATO_QT_ROOT}" STREQUAL "")
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{POTATO_QT_ROOT}")
endif()

option(POTATO_REQUIRE_EXACT_QT "Require the release-pinned Qt 6.8.8" OFF)
if(POTATO_REQUIRE_EXACT_QT)
    find_package(Qt6 6.8.8 EXACT REQUIRED COMPONENTS Core Gui Widgets Network Test)
else()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network Test)
endif()

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(Potato MACOSX_BUNDLE
    src/main.cpp
    src/app/AppController.cpp
    src/app/AppController.h
    src/app/AppNotifier.cpp
    src/app/AppNotifier.h
    src/app/SingleInstance.cpp
    src/app/SingleInstance.h
    src/settings/AppSettings.cpp
    src/settings/AppSettings.h
    src/settings/Localization.cpp
    src/settings/Localization.h
    src/settings/SettingsWindow.cpp
    src/settings/SettingsWindow.h
    src/settings/OnboardingWindow.cpp
    src/settings/OnboardingWindow.h
    src/settings/AboutWindow.cpp
    src/settings/AboutWindow.h
    src/pet/PetWindow.cpp
    src/pet/PetWindow.h
    src/pet/AnimationPlayer.cpp
    src/pet/AnimationPlayer.h
    src/pet/AnimationClip.cpp
    src/pet/AnimationClip.h
    src/pet/BehaviorController.cpp
    src/pet/BehaviorController.h
    src/pet/IdleActivityScheduler.cpp
    src/pet/IdleActivityScheduler.h
    src/pet/AtlasCache.cpp
    src/pet/AtlasCache.h
    src/pet/PetAtlas.cpp
    src/pet/PetAtlas.h
    src/pet/ClipContract.h
    src/resources/PackagePolicy.cpp
    src/resources/PackagePolicy.h
    src/resources/PetPackage.cpp
    src/resources/PetPackage.h
    src/resources/PetPackageValidator.cpp
    src/resources/PetPackageValidator.h
    src/resources/ArchiveExtractor.cpp
    src/resources/ArchiveExtractor.h
    src/resources/PetPackageImporter.cpp
    src/resources/PetPackageImporter.h
    src/resources/PetStore.cpp
    src/resources/PetStore.h
    src/resources/PetLibrary.cpp
    src/resources/PetLibrary.h
    src/settings/PetPreviewWidget.cpp
    src/settings/PetPreviewWidget.h
    src/quotes/QuoteProvider.cpp
    src/quotes/QuoteProvider.h
    src/quotes/SpeechBubble.cpp
    src/quotes/SpeechBubble.h
    src/input/InputActivitySource.h
    src/input/MacInputActivitySource.h
    src/input/MacInputActivitySource.mm
    src/input/TypingActivityDetector.cpp
    src/input/TypingActivityDetector.h
    src/environment/EnvironmentClock.cpp
    src/environment/EnvironmentClock.h
    src/environment/EnvironmentResolver.cpp
    src/environment/EnvironmentResolver.h
    src/environment/EnvironmentState.cpp
    src/environment/EnvironmentState.h
    src/accessibility/MotionController.cpp
    src/accessibility/MotionController.h
    src/platform/SystemActivitySource.h
    src/platform/MacSystemActivitySource.h
    src/platform/MacSystemActivitySource.mm
    src/login/LoginItemController.h
    src/login/LoginItemCoordinator.cpp
    src/login/LoginItemCoordinator.h
    src/login/MacLoginItemController.h
    src/login/MacLoginItemController.mm
    src/pet/WindowPlacement.cpp
    src/pet/WindowPlacement.h
    src/pet/PetGesture.cpp
    src/pet/PetGesture.h
    src/platform/MacApplication.h
    src/platform/MacApplication.mm
    src/platform/WindowOverlay.h
    src/platform/WindowOverlay.cpp
    src/platform/MacWindowOverlay.mm
)

target_include_directories(Potato PRIVATE src)
target_compile_options(Potato PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(Potato PRIVATE POTATO_VERSION="${PROJECT_VERSION}")

target_link_libraries(Potato PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    miniz
    "-framework AppKit"
    "-framework ApplicationServices"
    "-framework ServiceManagement"
)

add_executable(PotatoLoginHelper MACOSX_BUNDLE
    src/login/LoginHelperMain.mm
)
target_link_libraries(PotatoLoginHelper PRIVATE "-framework AppKit")
target_compile_options(PotatoLoginHelper PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(PotatoLoginHelper PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "PotatoLoginHelper"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.peng.potato.login-helper"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LoginHelper-Info.plist.in"
)

add_dependencies(Potato PotatoLoginHelper)
add_custom_command(TARGET Potato POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Library/LoginItems"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:PotatoLoginHelper>"
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Library/LoginItems/PotatoLoginHelper.app"
    VERBATIM
)

add_custom_command(TARGET Potato POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Skills"
    COMMAND ${CMAKE_COMMAND}
            "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/third_party/hatch-pet"
            "-DDESTINATION=$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Skills/hatch-pet"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyHatchPet.cmake"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Documentation"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/docs/PET_AUTHORING.md"
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Documentation/PET_AUTHORING.md"
    COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Pets"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Pets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/Pets"
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Pets"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Licenses/miniz"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz/LICENSE"
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Licenses/miniz/LICENSE"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Licenses/hatch-pet"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/hatch-pet/LICENSE.txt"
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/Licenses/hatch-pet/LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
            "$<TARGET_BUNDLE_DIR:Potato>/Contents/Resources/THIRD_PARTY_NOTICES.md"
    VERBATIM
)

set_target_properties(Potato PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Potato"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.peng.potato"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
)

option(POTATO_BUILD_TESTS "Build Potato's automated tests" ON)
if(POTATO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
