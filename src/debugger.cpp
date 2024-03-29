#include "nkgt/debugger.hpp"
#include "nkgt/error_codes.hpp"
#include "nkgt/registers.hpp"
#include "nkgt/util.hpp"

#include <cstdint>
#include <libdwarf.h>
#include <linenoise.h>
#include <fmt/core.h>
#include <tl/expected.hpp>

#include <cerrno>
#include <charconv>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unordered_map>

namespace {

auto wait_for_signal(pid_t pid) -> void {
    int wait_status = 0;
    int options = 0;

    waitpid(pid, &wait_status, options);
}

auto step_over_breakpoint(
    pid_t pid,
    std::unordered_map<std::intptr_t, nkgt::debugger::breakpoint>& breakpoint_list
) -> bool {
    const auto current_pc = nkgt::registers::get_register_value(pid, nkgt::registers::reg::rip);

    if(!current_pc) {
        fmt::print("Failed to get current Program Counter value.\n");
        return false;
    }

    uint64_t possible_bp_location = *current_pc - 1;

    const auto& bp_it = breakpoint_list.find(possible_bp_location);
    if(bp_it != breakpoint_list.cend() && bp_it->second.enabled) {
        const auto pc_result = nkgt::registers::set_register_value(
            pid,
            nkgt::registers::reg::rip,
            possible_bp_location
        );

        if(!pc_result) {
            fmt::print("Failed to set Program Counter value.\n");
            return false;
        }

        nkgt::debugger::breakpoint& bp = bp_it->second;

        const auto bp_result = nkgt::debugger::disable_breakpoint(bp);
        if(!bp_result) {
            fmt::print("Failed to disable breakpoint at {}.\n", bp.address);
            return false;
        }

        if(ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr) == -1) {
            nkgt::util::print_error_message("ptrace", errno);
            return false;
        }

        wait_for_signal(pid);

        const auto set_bp_result = nkgt::debugger::enable_breakpoint(bp);
        if(!set_bp_result) {
            fmt::print("Failed to re-enable breakpoint at {}.\n", bp.address);
            return false;
        }
    }
    
    return true;
}

auto continue_execution(
    pid_t pid,
    std::unordered_map<std::intptr_t, nkgt::debugger::breakpoint>& breakpoint_list
) -> void {
    const bool result = step_over_breakpoint(pid, breakpoint_list);

    if(!result) {
        fmt::print("Failed to step over breakpoint. Continuing execution with in unknow state\n");
    }

    if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1) {
        nkgt::util::print_error_message("ptrace", errno);
        return;
    }
    
    int wait_status = 0;
    int options = 0;
    waitpid(pid, &wait_status, options);
}

template<typename T>
auto hex_from_str(
    std::string_view address_str
) -> tl::expected<T, nkgt::error::address> {
    if(address_str.substr(0, 2) != "0x") {
        fmt::print("HEX argument to command should start with 0x.\n");
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

auto try_set_breakpoint(
    std::string_view address_str,
    pid_t pid,
    std::unordered_map<std::intptr_t, nkgt::debugger::breakpoint>& breakpoint_list
) -> void {
    const auto address = hex_from_str<std::intptr_t>(address_str);

    if(!address) {
        fmt::print("Failed to parse address.\n");
        return;
    }

    if(breakpoint_list.find(*address) != breakpoint_list.cend()) {
        fmt::print("Breakpoint already active at {}.\n", address_str);
        return;
    }

    nkgt::debugger::breakpoint bp = {pid, *address};
    const auto result = enable_breakpoint(bp);

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

    breakpoint_list[*address] = bp;
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
    const auto reg = nkgt::registers::from_string(reg_str);

    if(!reg) {
        switch(reg.error()) {
        case nkgt::error::registers::unknown_reg_name:
            fmt::print("{} is not the name of a register.\n", reg_str);
            return;
        default:
            fmt::print("Unknown error while paring the register name {}.\n", reg_str);
            return;
        }
    }

    const auto value = nkgt::registers::get_register_value(pid, *reg);

    if(!value) {
        switch(value.error()) {
        case nkgt::error::registers::getregs_fail:
            fmt::print("Failed to get retrieve the value of the register {}.\n", reg_str);
            return;
        default:
            fmt::print("Unknown error while trying to retrieve the value for the register {}.\n", reg_str);
            return;
        }
    }

    fmt::print("{} = {:#018x}\n", reg_str, *value);
    return;
}

auto handle_break_command(
    std::vector<std::string_view> args,
    pid_t pid,
    std::unordered_map<std::intptr_t, nkgt::debugger::breakpoint>& breakpoint_list
) -> void {
    if(args.size() != 2) {
        fmt::print(
            "Wrong number of arguments for register command {}. Allowed usages are\n"
            "\tbreak address\n",
            "break"
        );

        return;
    }

    try_set_breakpoint(args[1], pid, breakpoint_list);
    return;
}

auto handle_register_command(
    std::vector<std::string_view> args,
    pid_t pid
) -> void {
    if(args.size() == 2 && nkgt::util::is_prefix(args[1], "dump")) {
        nkgt::registers::dump_registers(pid);
    } else if (args.size() == 3 && nkgt::util::is_prefix(args[1], "read")) {
        try_read_register(args[2], pid);
    } else if (args.size() == 4 && nkgt::util::is_prefix(args[1], "write")) {
        try_set_register(args[3], args[2], pid);
    } else {
        fmt::print(
            "Wrong number of arguments for register command {}. Allowed usages are\n"
            "\tregister dump\n"
            "\tregister read register_name\n"
            "\tregister write register_name value\n",
            "register"
        );
    }
}

// Parses the user input and then dispatches to the appropriate command logic.
// Return true if the "quit" command has been issued, false otherwise.
auto handle_command(
    const std::string& line,
    pid_t pid,
    std::unordered_map<std::intptr_t, nkgt::debugger::breakpoint>& breakpoint_list
) -> bool {
    std::vector<std::string_view> args = nkgt::util::split(line, ' ');

    if(args.empty()) {
        return false;
    }

    std::string_view command = args[0];

    if(nkgt::util::is_prefix(command, "continue")) {
        continue_execution(pid, breakpoint_list);
    } else if(nkgt::util::is_prefix(command, "break")) {
        handle_break_command(args, pid, breakpoint_list);
    } else if(nkgt::util::is_prefix(command, "register")) {
        handle_register_command(args, pid);
    } else if(nkgt::util::is_prefix(command, "quit")) {
        return true;
    } else {
        fmt::print("Unknow command\n");
    }

    return false;
}

[[nodiscard]]
auto init_debug_symbols(
    const std::filesystem::path& program_path
) -> tl::expected<Dwarf_Debug, nkgt::error::debug_symbols> {
    char actual_path[FILENAME_MAX];
    Dwarf_Handler error_handler = 0;
    Dwarf_Ptr error_arg = 0;
    Dwarf_Error error = 0;
    Dwarf_Debug dbg = 0;

    int result = dwarf_init_path(
        program_path.c_str(),
        actual_path,
        FILENAME_MAX,
        DW_GROUPNUMBER_ANY,
        error_handler,
        error_arg,
        &dbg,
        &error
    );

    if(result == DW_DLV_ERROR) {
        dwarf_dealloc_error(dbg, error);
        return tl::make_unexpected(nkgt::error::debug_symbols::load_fail);
    }

    if(result == DW_DLV_NO_ENTRY) {
        return tl::make_unexpected(nkgt::error::debug_symbols::load_fail);
    }

    fmt::print("Loaded symbols from path: {}", actual_path);
    return dbg;
}

// Taken from https://www.prevanders.net/libdwarfdoc/group__examplecuhdr.html
[[nodiscard]]
auto get_func_die_from_pc(
    const Dwarf_Debug& symbols,
    std::uint64_t pc
) -> int {
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Half offset_size = 0;
    Dwarf_Half extension_size = 0;
    Dwarf_Sig8 signature;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Half header_cu_type = 0;
    Dwarf_Bool is_info = 1;
    Dwarf_Error error;

    bool function_found = false;
    int result = 0;

    while(!function_found) {
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;
        Dwarf_Unsigned cu_header_length = 0;

        result = dwarf_next_cu_header_d(
            symbols,
            is_info,
            &cu_header_length,
            &version_stamp,
            &abbrev_offset,
            &address_size,
            &offset_size,
            &extension_size,
            &signature,
            &typeoffset,
            &next_cu_header,
            &header_cu_type,
            &error
        );

        if(result == DW_DLV_ERROR) {
            // TODO
        }

        if(result == DW_DLV_NO_ENTRY) {
            if(is_info) {
                fmt::print("Finished searching pc {} in .debug_info. Switching to .debug_types\n", pc);
                is_info = 0;
            }

            fmt::print("Finished searching for pc {}\n", pc);
            return result;
        }

        result = dwarf_siblingof_b(
            symbols,
            no_die,
            is_info,
            &cu_die,
            &error
        );

        if(result == DW_DLV_ERROR) {
            //
        }

        if(result == DW_DLV_NO_ENTRY) {
            // FATAL ERROR
        }

        // CHECK FOR FUNCTION PC!
    }

    return result;
}

}

namespace nkgt::debugger {

// In x86 0xcc is the instruction that identifies a breakpoint. So enabling
// a breakpoint simply means replacing whatever instruction is at
// bp.address with 0xcc.
[[nodiscard]]
auto enable_breakpoint(
    breakpoint& bp
) -> tl::expected<void, error::breakpoint> {
    // Calling ptrace with PTRACE_PEEKDATA retrieves the word at bp.address for
    // the process identified by bp.pid. This means that -1 is a valid return
    // value and does not necessarily describe an error. To solve this we clear
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
[[nodiscard]]
auto disable_breakpoint(
    breakpoint& bp
) -> tl::expected<void, error::breakpoint> {
    // See comment in enable_breakpoint() for more info.
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

auto run(
    pid_t pid,
    const std::filesystem::path& program_path
) -> void {
    // wait for the child process to finish launching the program we want to debug
    wait_for_signal(pid);

    // Setting the option PTRACE_O_EXITKILL to the debugee ensures that it will
    // exit when the debugger itself exits.
    if(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_EXITKILL) == -1) {
        util::print_error_message("ptrace", errno);
        return;
    }

    const auto symbols = init_debug_symbols(program_path);

    if(!symbols) {
        fmt::print("Failed to load the debug symbols.\n");
        return;
    }

    std::unordered_map<std::intptr_t, breakpoint> breakpoint_list;

    char* line = nullptr;
    while((line = linenoise("dbg> ")) != nullptr) {
        if(handle_command(line, pid, breakpoint_list)) {
            linenoiseFree(line);
            break;
        }

        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }

    dwarf_finish(*symbols);
    return;
}

}
