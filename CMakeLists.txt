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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "Potato only supports Apple Silicon")
set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "Minimum supported macOS version")

if(DEFINED ENV{POTATO_QT_ROOT} AND NOT "$ENV{POTATO_QT_ROOT}" STREQUAL "")
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{POTATO_QT_ROOT}")
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network Test)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(Potato MACOSX_BUNDLE
    src/main.cpp
    src/app/AppController.cpp
    src/app/AppController.h
    src/app/SingleInstance.cpp
    src/app/SingleInstance.h
    src/settings/AppSettings.cpp
    src/settings/AppSettings.h
    src/settings/Localization.cpp
    src/settings/Localization.h
    src/settings/SettingsWindow.cpp
    src/settings/SettingsWindow.h
    src/pet/PetWindow.cpp
    src/pet/PetWindow.h
    src/pet/AnimationPlayer.cpp
    src/pet/AnimationPlayer.h
    src/pet/BehaviorController.cpp
    src/pet/BehaviorController.h
    src/pet/AtlasCache.cpp
    src/pet/AtlasCache.h
    src/pet/PetAtlas.cpp
    src/pet/PetAtlas.h
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
    src/pet/WindowPlacement.cpp
    src/pet/WindowPlacement.h
    src/platform/MacApplication.h
    src/platform/MacApplication.mm
)

target_include_directories(Potato PRIVATE src)

target_link_libraries(Potato PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    miniz
    "-framework AppKit"
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
