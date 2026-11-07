cmake_minimum_required(VERSION 3.16)

project(C3350EMU LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(c3350_core
    src/cpu/cpu.cpp
    src/firmware/fls_parser.cpp
    src/firmware/boot_plan.cpp
    src/memory/memory.cpp
    src/memory/lpddr1.cpp
    src/platform/platform.cpp
    src/platform/battery.cpp
)

target_include_directories(c3350_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(c3350emu src/main.cpp)
target_link_libraries(c3350emu PRIVATE c3350_core)

if(WIN32)
    add_executable(c3350emu-gui WIN32 src/gui/main.cpp)
    target_link_libraries(c3350emu-gui PRIVATE c3350_core comdlg32)
else()
    add_executable(c3350emu-gui src/gui/linux_stub.cpp)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(test_lpddr1 tests/test_lpddr1.cpp)
    target_link_libraries(test_lpddr1 PRIVATE c3350_core)
    add_test(NAME lpddr1_protocol_tests COMMAND test_lpddr1)

    add_executable(test_memory tests/test_memory.cpp)
    target_link_libraries(test_memory PRIVATE c3350_core)
    add_test(NAME memory_tests COMMAND test_memory)

    add_executable(test_boot_plan tests/test_boot_plan.cpp)
    target_link_libraries(test_boot_plan PRIVATE c3350_core)
    add_test(NAME boot_plan_tests COMMAND test_boot_plan)

    add_executable(test_fls_parser tests/test_fls_parser.cpp)
    target_link_libraries(test_fls_parser PRIVATE c3350_core)
    add_test(NAME fls_parser_tests COMMAND test_fls_parser)

    add_executable(test_platform tests/test_platform.cpp)
    target_link_libraries(test_platform PRIVATE c3350_core)
    add_test(NAME platform_tests COMMAND test_platform)

    add_executable(test_battery tests/test_battery.cpp)
    target_link_libraries(test_battery PRIVATE c3350_core)
    add_test(NAME battery_tests COMMAND test_battery)

    add_executable(test_cpu tests/test_cpu.cpp)
    target_link_libraries(test_cpu PRIVATE c3350_core)
    add_test(NAME cpu_tests COMMAND test_cpu)
endif()
