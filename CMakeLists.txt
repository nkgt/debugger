cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0048 NEW)
project(
    debugger
    VERSION 0.0.0
    LANGUAGES CXX
)

cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)

include(cmake/compiler_flags.cmake)
include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        9.1.0
)

FetchContent_Declare(
    linenoise
    GIT_REPOSITORY https://github.com/nkgt/linenoise
    GIT_TAG        master
)

set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE_DEB OFF)
FetchContent_Declare(
    expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected
    GIT_TAG        master
)

FetchContent_Declare(
    dwarf
    GIT_REPOSITORY https://github.com/davea42/libdwarf-code
    GIT_TAG        v0.6.0
)

FetchContent_MakeAvailable(fmt linenoise expected dwarf)

add_library(debugger
    src/debugger.cpp
    src/util.cpp
    src/registers.cpp
)
target_include_directories(debugger PUBLIC include)
target_link_libraries(debugger PRIVATE fmt::fmt linenoise expected dwarf-static)
set_compiler_flags(debugger)

add_executable(dbg frontend/main.cpp)
target_link_libraries(dbg PRIVATE fmt::fmt debugger expected)
set_compiler_flags(dbg)

if(DEBUGGER_EXAMPLE)
    add_executable(simple example/simple.cpp)
    target_compile_options(simple PRIVATE -Og)
endif()

if(DEBUGGER_TESTS)
    add_subdirectory(tests)
endif()
