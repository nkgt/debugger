#pragma once
#include <cstdint>
#include <sys/types.h>

#include <tl/expected.hpp>

#include "nkgt/error_codes.hpp"

namespace nkgt::debugger {

struct breakpoint {
    pid_t pid;
    std::intptr_t address;
    bool enabled = false;
    uint8_t saved_data = 0;
};

tl::expected<void, error::breakpoint> enable_brakpoint(breakpoint& bp);
tl::expected<void, error::breakpoint> disable_brakpoint(breakpoint& bp);

void run(pid_t pid);

}
