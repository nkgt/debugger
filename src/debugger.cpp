#include "nkgt/debugger.hpp"
#include "nkgt/util.hpp"

#include <cerrno>
#include <linenoise.h>
#include <fmt/core.h>

#include <sys/ptrace.h>
#include <sys/wait.h>

namespace nkgt::debugger {

static void continue_execution(pid_t pid) {
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    
    int wait_status = 0;
    int options = 0;
    waitpid(pid, &wait_status, options);
}

static void handle_command(const std::string& line, pid_t pid) {
    std::vector<std::string_view> args = nkgt::util::split(line, ' ');

    if(args.empty()) {
        return;
    }

    std::string_view command = args[0];

    if(nkgt::util::is_prefix(command, "continue")) {
        continue_execution(pid);
    } else {
        fmt::print("Unknow command\n");
    }
}

// In x86 0xcc is the instruction that identifies a breakpoint. So enabling
// a breakpoint simply means replacing whatever instruction is at
// bp.address with 0xcc.
void enable_brakpoint(breakpoint& bp) {
    // Calling ptrace with PTRACE_PEEKDATA retrieves the word at bp.address for
    // the process identified by bp.pid. This means that -1 is a valid return
    // value and does not necesserely describe an error. To solve this we clear
    // errno before calling ptrace and we check it right after.
    // More info at RETURN_VALUE in ptrace(2).
    errno = 0;
    long data = ptrace(PTRACE_PEEKDATA, bp.pid, bp.address, nullptr);
    if(data == -1 && errno != 0) {
        util::print_error_message("ptrace", errno);
        return;
    }


    long data_with_trap = ((data & ~0xff) | 0xcc);
    bp.saved_data = static_cast<uint8_t>(data & 0xff);
    
    if(ptrace(PTRACE_POKEDATA, bp.pid, bp.address, data_with_trap) == -1) {
        util::print_error_message("ptrace", errno);
        return;
    }

    bp.enabled = true;
}

// Disables a breakpoint by restoring the original data (bp.saved_data) in the
// location bp.address.
void disable_brakpoint(breakpoint& bp) {
    // See comment in enable_brakpoint() for more info.
    errno = 0;
    long data = ptrace(PTRACE_PEEKDATA, bp.pid, bp.address, nullptr);
    if(data == -1 && errno != 0) {
        util::print_error_message("ptrace", errno);
        return;
    }

    long restored_data = ((data & ~0xff) | bp.saved_data);

    if(ptrace(PTRACE_POKEDATA, bp.pid, bp.address, restored_data) == -1) {
        util::print_error_message("ptrace", errno);
        return;
    }

    bp.enabled = false;
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