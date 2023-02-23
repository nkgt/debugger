#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include "nkgt/util.hpp"

TEST_CASE("String are correctly split", "[util]") {
    using Catch::Matchers::SizeIs;
    using Catch::Matchers::RangeEquals;

    SECTION("Empty strings results in an empty range") {
        REQUIRE_THAT(
            nkgt::util::split("", ' '), 
            SizeIs(0)
        );
    }

    SECTION("Starting delimiter characters are ignored") {
        REQUIRE_THAT(
            nkgt::util::split("   aa", ' '),
            SizeIs(1) && RangeEquals(std::vector<const char*>({"aa"}))
        );
    }

    SECTION("Trailing delimiter characters are ignored") {
        REQUIRE_THAT(
            nkgt::util::split("aaa     ", ' '),
            SizeIs(1) && RangeEquals(std::vector<const char*>({"aaa"}))
        );
    }

    SECTION("Starting and trailing delimiter characters, just one word") {
        REQUIRE_THAT(
            nkgt::util::split("  aaa     ", ' '),
            SizeIs(1) && RangeEquals(std::vector<const char*>({"aaa"}))
        );
    }

    SECTION("Full example") {
        REQUIRE_THAT(
            nkgt::util::split("  f 0909 !34j  0-09    aaa     ", ' '),
            SizeIs(5) && RangeEquals(std::vector<const char*>({"f", "0909", "!34j", "0-09", "aaa"}))
        );
    }
}

TEST_CASE("Prefixes are correctly identified", "[util]") {
    SECTION("Actual prefix returns true") {
        REQUIRE(nkgt::util::is_prefix("c", "continue"));
    }

    SECTION("A prefix longer than the full string returns false") {
        REQUIRE_FALSE(nkgt::util::is_prefix("continue_", "continue"));
    }

    SECTION("Empty prefix retruns false") {
        REQUIRE_FALSE(nkgt::util::is_prefix("", "continue"));
    }

    SECTION("Empty full string is a failure") {
        REQUIRE_FALSE(nkgt::util::is_prefix("c", ""));
    }
}
