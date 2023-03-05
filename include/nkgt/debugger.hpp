#pragma once
#include <cstdint>
#include <sys/types.h>

namespace nkgt::debugger {

struct breakpoint {
    pid_t pid;
    std::intptr_t address;
    bool enabled;
    uint8_t saved_data;
};

void enable_brakpoint(breakpoint& bp);
void disable_brakpoint(breakpoint& bp);

void run(pid_t pid);

}