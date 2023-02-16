cmake_policy(SET CMP0048 NEW)
project(
    debugger
    VERSION 0.0.0
    LANGUAGES CXX
)

cmake_minimum_required(VERSION 3.22)
set(CXX_STANDARD 17)

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

add_executable(debugger src/main.cpp src/debugger.cpp src/util.cpp)
target_include_directories(debugger PRIVATE include)
target_link_libraries(debugger PRIVATE spdlog::spdlog linenoise)
target_compile_options(debugger PRIVATE -Wall -Wextra -std=c++17)

if(DEBUGGER_EXAMPLE)
    add_executable(simple example/simple.cpp)
    target_compile_options(simple PRIVATE -g -O0)
endif()
