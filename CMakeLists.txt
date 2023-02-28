cmake_policy(SET CMP0048 NEW)
project(
    debugger
    VERSION 0.0.0
    LANGUAGES CXX
)

cmake_minimum_required(VERSION 3.22)
set(CXX_STANDARD 17)

include(cmake/compiler_flags.cmake)
include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        v1.11.0
)

FetchContent_Declare(
    linenoise
    GIT_REPOSITORY https://github.com/nkgt/linenoise
    GIT_TAG        master
)

FetchContent_MakeAvailable(spdlog linenoise)

add_library(debugger src/debugger.cpp src/util.cpp)
target_include_directories(debugger PUBLIC include)
target_link_libraries(debugger PRIVATE spdlog::spdlog linenoise)
set_compiler_flags(debugger)

add_executable(dbg frontend/main.cpp)
target_link_libraries(dbg PRIVATE spdlog::spdlog debugger)
set_compiler_flags(dbg)

if(DEBUGGER_EXAMPLE)
    add_executable(simple example/simple.cpp)
    target_compile_options(simple PRIVATE -Og)
endif()

if(DEBUGGER_TESTS)
    add_subdirectory(tests)
endif()
