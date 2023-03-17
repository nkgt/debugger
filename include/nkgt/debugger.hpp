#pragma once
#include "nkgt/error_codes.hpp"

#include <tl/expected.hpp>

#include <cstdint>
#include <sys/types.h>

namespace nkgt::debugger {

struct breakpoint {
    pid_t pid;
    std::intptr_t address;
    bool enabled = false;
    uint8_t saved_data = 0;
};

tl::expected<void, error::breakpoint> enable_breakpoint(breakpoint& bp);
tl::expected<void, error::breakpoint> disable_breakpoint(breakpoint& bp);

void run(pid_t pid);

}
