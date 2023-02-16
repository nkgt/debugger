#pragma once

#include <vector>
#include <string_view>

namespace nkgt::util {

// Returns an array of string_view containing the substrings delimited by a
// sequence of delimiter characters in O(source.size()) time. Note that, if
// multiple delimiterscharacters are found one after the other, they are all
// skipped and not tokenized.
//
// Example: split("  a   bb ", ' ') -> {"a", "bb"}
std::vector<std::string_view> split(std::string_view source, char delimiter);

// Check if prefix is a prefix of full. That is, if of is equal to or starts
// with s.
bool is_prefix(std::string_view prefix, std::string_view full);

}
