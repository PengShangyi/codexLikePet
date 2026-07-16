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
