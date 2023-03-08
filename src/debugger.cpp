#include "nkgt/debugger.hpp"
#include "nkgt/error_codes.hpp"
#include "nkgt/util.hpp"

#include <linenoise.h>
#include <fmt/core.h>

#include <cerrno>
#include <charconv>
#include <sys/ptrace.h>
#include <sys/wait.h>

namespace nkgt::debugger {

static void continue_execution(pid_t pid) {
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    
    int wait_status = 0;
    int options = 0;
    waitpid(pid, &wait_status, options);
}

static void try_set_breakpoint(std::string_view address_str, pid_t pid) {
    if(address_str.substr(0, 2) != "0x") {
        fmt::print("Address argument to break command should start with 0x.\n");
        return; 
    }

    long address = 0;
    auto [_, ec] = std::from_chars(
        address_str.data() + 2, 
        address_str.data() + address_str.size(), 
        address,
        16
    );

    if(ec != std::errc()) {
        fmt::print("Invalid argument passed to break command.\n");
        return;
    }

    breakpoint bp = {pid, address};
    const auto result = enable_brakpoint(bp);

    if(!result) {
        switch(result.error()) {
        case error::breakpoint::peek_address_fail:
            fmt::print(
                "Failed to retrieve instruction at address {} for PID {}",
                address,
                pid
            );
            break;
        case error::breakpoint::poke_address_fail:
            fmt::print(
                "Failed to modify instruction at address {} for PID {}",
                address,
                pid
            );
            break;
        }
    }
}

static void handle_command(const std::string& line, pid_t pid) {
    std::vector<std::string_view> args = nkgt::util::split(line, ' ');

    if(args.empty()) {
        return;
    }

    std::string_view command = args[0];

    if(nkgt::util::is_prefix(command, "continue")) {
        continue_execution(pid);
    } else if(util::is_prefix(command, "break")) {
        if(args.size() == 1) {
            fmt::print("Missing argument for break command.\n");
            return;
        }

        if(args.size() > 2) {
            fmt::print("Too many arguments for break command.\n");
            return;
        }

        try_set_breakpoint(args[1], pid);
    } else {
        fmt::print("Unknow command\n");
    }
}

// In x86 0xcc is the instruction that identifies a breakpoint. So enabling
// a breakpoint simply means replacing whatever instruction is at
// bp.address with 0xcc.
tl::expected<void, error::breakpoint> enable_brakpoint(breakpoint& bp) {
    // Calling ptrace with PTRACE_PEEKDATA retrieves the word at bp.address for
    // the process identified by bp.pid. This means that -1 is a valid return
    // value and does not necesserely describe an error. To solve this we clear
    // errno before calling ptrace and we check it right after.
    // More info at RETURN_VALUE in ptrace(2).
    errno = 0;
    long data = ptrace(PTRACE_PEEKDATA, bp.pid, bp.address, nullptr);
    if(data == -1 && errno != 0) {
        util::print_error_message("ptrace", errno);
        return tl::unexpected(error::breakpoint::peek_address_fail);
    }

    long data_with_trap = ((data & ~0xff) | 0xcc);
    if(ptrace(PTRACE_POKEDATA, bp.pid, bp.address, data_with_trap) == -1) {
        util::print_error_message("ptrace", errno);
        return tl::unexpected(error::breakpoint::poke_address_fail);
    }

    // The breakpoint is modified at the end so that in case of errors there is
    // no leftover data in it.
    bp.saved_data = static_cast<uint8_t>(data & 0xff);
    bp.enabled = true;

    return {};
}

// Disables a breakpoint by restoring the original data (bp.saved_data) in the
// location bp.address.
tl::expected<void, error::breakpoint> disable_brakpoint(breakpoint& bp) {
    // See comment in enable_brakpoint() for more info.
    errno = 0;
    long data = ptrace(PTRACE_PEEKDATA, bp.pid, bp.address, nullptr);
    if(data == -1 && errno != 0) {
        util::print_error_message("ptrace", errno);
        return tl::unexpected(error::breakpoint::peek_address_fail);
    }

    long restored_data = ((data & ~0xff) | bp.saved_data);

    if(ptrace(PTRACE_POKEDATA, bp.pid, bp.address, restored_data) == -1) {
        util::print_error_message("ptrace", errno);
        return tl::unexpected(error::breakpoint::poke_address_fail);
    }

    bp.enabled = false;

    return {};
}

void run(pid_t pid) {
    int wait_status = 0;
    int options = 0;

    // wait for the child process to finish launching the programm we want to debug
    waitpid(pid, &wait_status, options);

    char* line = nullptr;
    while((line = linenoise("dbg> ")) != nullptr) {
        handle_command(line, pid);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

}
