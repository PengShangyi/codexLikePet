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
