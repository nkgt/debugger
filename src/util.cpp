#include "nkgt/util.hpp"

namespace nkgt::util {

std::vector<std::string_view> split(std::string_view s, char delimiter) {
    // If s is empty we have that min == s.size() == 0 so that the while is
    // skipped and we return {} in the following if.
    std::size_t min = 0;
    while (min < s.size() && s[min] == delimiter) {
        min += 1;
    }

    if (min == s.size()) {
        return {};
    }

    std::size_t max = s.size() - 1;
    while (max > min && s[max] == delimiter) {
        max -= 1;
    }

    std::size_t token_count = 1;
    for (std::size_t i = min; i <= max; ++i) {
        if (s[i] == delimiter) {
            token_count += 1;

            // There is no need to check if i exceeds max since we trimmed the
            // string so that it ends with a character different from delimiter
            // and therefore the following check will always fail before i
            // becomes bigger than max.
            while (s[i] == delimiter) {
                i += 1;
            }
        }
    }

    std::vector<std::string_view> tokens;
    tokens.reserve(token_count);
    std::size_t begin = min;

    for (std::size_t i = min; i <= max; ++i) {
        if (s[i] == delimiter) {
            tokens.emplace_back(s.data() + begin, i - begin);

            while (s[i] == delimiter) {
                i += 1;
            }

            begin = i;
        }
    }

    tokens.emplace_back(s.data() + begin, max - begin + 1);

    return tokens;
}

bool is_prefix(std::string_view s, std::string_view of) {
    if(s.size() > of.size()) {
        return false;
    }

    for(std::size_t i = 0; i < s.size(); ++i) {
        if(s[i] != of[i]) {
            return false;
        }
    }

    return true;
}

}
