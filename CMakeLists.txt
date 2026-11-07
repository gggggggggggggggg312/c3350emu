cmake_minimum_required(VERSION 3.16)

project(C3350EMU LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(c3350_cpu
    src/cpu/cpu.cpp
)

target_include_directories(c3350_cpu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(c3350emu
    src/main.cpp
)

target_link_libraries(c3350emu PRIVATE c3350_cpu)
