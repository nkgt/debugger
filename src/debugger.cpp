#include "nkgt/debugger.hpp"
#include "nkgt/error_codes.hpp"
#include "nkgt/registers.hpp"
#include "nkgt/util.hpp"
#include "tl/expected.hpp"

#include <linenoise.h>
#include <fmt/core.h>

#include <cerrno>
#include <charconv>
#include <sys/ptrace.h>
#include <sys/wait.h>

namespace {

void continue_execution(pid_t pid) {
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    
    int wait_status = 0;
    int options = 0;
    waitpid(pid, &wait_status, options);
}

template<typename T>
auto hex_from_str(
    std::string_view address_str
) -> tl::expected<T, nkgt::error::address> {
    if(address_str.substr(0, 2) != "0x") {
        fmt::print("Address argument to break command should start with 0x.\n");
        return tl::make_unexpected(nkgt::error::address::malformed_register);
    }

    T address = 0;
    auto [_, ec] = std::from_chars(
        address_str.data() + 2, 
        address_str.data() + address_str.size(), 
        address,
        16
    );

    if(ec != std::errc()) {
        fmt::print("Invalid argument passed to break command.\n");
        return tl::make_unexpected(nkgt::error::address::malformed_register);
    }

    return address;
}

void try_set_breakpoint(std::string_view address_str, pid_t pid) {
    const auto address = hex_from_str<std::intptr_t>(address_str);

    if(!address) {
        fmt::print("Failed to parse address.\n");
        return;
    }

    nkgt::debugger::breakpoint bp = {pid, *address};
    const auto result = enable_brakpoint(bp);

    if(!result) {
        switch(result.error()) {
        case nkgt::error::breakpoint::peek_address_fail:
            fmt::print(
                "Failed to retrieve instruction at address {} for PID {}",
                *address,
                pid
            );
            break;
        case nkgt::error::breakpoint::poke_address_fail:
            fmt::print(
                "Failed to modify instruction at address {} for PID {}",
                *address,
                pid
            );
            break;
        }
    }
}

auto try_set_register(
    std::string_view value_str,
    std::string_view reg_str,
    pid_t pid
) -> void {
    const auto value = hex_from_str<uint64_t>(value_str);
    
    if(!value) {
        fmt::print("Failed to parse value.\n");
        return;
    }

    const auto reg = nkgt::registers::from_string(reg_str);

    if(!reg) {
        fmt::print("Unknown register name {}\n", reg_str);
        return;
    }

    const auto result = nkgt::registers::set_register_value(pid, *reg, *value);

    if(!result) {
        fmt::print("Failed to set the value for the register {}", reg_str);
    }
}

auto try_read_register(
    std::string_view reg_str,
    pid_t pid
) -> void {
    const auto result = nkgt::registers::from_string(reg_str)
        .and_then([pid](auto&& reg) {
            return nkgt::registers::get_register_value(pid, reg);
        }).map([reg_str](auto&& value) {
            fmt::print("{} = {:#018x}\n", reg_str, value);
            return;
        });

    if(!result) {
        switch(result.error()) {
        case nkgt::error::registers::unknown_reg_name:
            fmt::print("Unknown register name {}\n", reg_str);
            return;
        case nkgt::error::registers::getregs_fail:
            fmt::print("Failed to retrive the register value\n");
            return;
        default:
            fmt::print("");
            return;
        }
    }
}

void handle_command(const std::string& line, pid_t pid) {
    std::vector<std::string_view> args = nkgt::util::split(line, ' ');

    if(args.empty()) {
        return;
    }

    std::string_view command = args[0];

    if(nkgt::util::is_prefix(command, "continue")) {
        continue_execution(pid);
    } else if(nkgt::util::is_prefix(command, "break")) {
        if(args.size() == 1) {
            fmt::print("Missing argument for break command.\n");
            return;
        }

        if(args.size() > 2) {
            fmt::print("Too many arguments for break command.\n");
            return;
        }

        try_set_breakpoint(args[1], pid);
    } else if(nkgt::util::is_prefix(command, "register")) {
        if(args.size() == 2 && args[1] == "dump") {
            nkgt::registers::dump_registers(pid);
        } else if (args.size() == 3 && args[1] == "read") {
            try_read_register(args[2], pid);
        } else if (args.size() == 4 && args[1] == "write") {
            try_set_register(args[3], args[2], pid);
        }
    } else {
        fmt::print("Unknow command\n");
    }
}
}

namespace nkgt::debugger {

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
