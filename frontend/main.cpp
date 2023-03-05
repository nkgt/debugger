#include <sys/ptrace.h>
#include <sys/types.h>
#include <unistd.h>

#include <fmt/core.h>

#include "nkgt/debugger.hpp"
#include "nkgt/util.hpp"

int main(int argc, const char** argv) {
    if(argc < 2) {
        fmt::print("Program name not specified!");
        return -1;
    }

    const char* program_name = argv[1];
    pid_t pid = fork();

    if(pid == 0) {
        if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
            nkgt::util::print_error_message("ptrace", errno);
            return -1;
        }

        if(execl(program_name, program_name, nullptr) == -1) {
            nkgt::util::print_error_message("execl", errno);
            return -1;
        }
    } else if(pid >= 1) {
        nkgt::debugger::run(pid);
    } else {
        nkgt::util::print_error_message("fork", errno);
        return -1;
    }

    return 0;
}
