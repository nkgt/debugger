FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.3.1
)

FetchContent_MakeAvailable(catch2)
# Needed in order to use include(Catch) below
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_executable(debugger_tests util_tests.cpp)
target_link_libraries(debugger_tests PRIVATE debugger Catch2::Catch2WithMain)
set_compiler_flags(debugger_tests)

include(CTest)
include(Catch)
catch_discover_tests(debugger_tests)
