#pragma once

#include <vector>
#include <string_view>

namespace nkgt::util {

// Simple wrapper to format an error message. error_number should be a copy of
// errno right after the call that failed and before any other function call.
// For more detail see NOTES in errno(3).
void print_error_message(const char* procedure_name, int error_number);

// Returns an array of string_view containing the substrings delimited by a
// sequence of delimiter characters in O(source.size()) time. Note that, if
// multiple delimiterscharacters are found one after the other, they are all
// skipped and not tokenized.
//
// Example: split("  a   bb ", ' ') -> {"a", "bb"}
std::vector<std::string_view> split(std::string_view source, char delimiter);

// Check if prefix is a prefix of full. That is, if full is equal to or starts
// with prefix.
bool is_prefix(std::string_view prefix, std::string_view full);

}
