#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

#include "nkgt/debugger.hpp"

void print_error_message(const char* procedure_name) {
    spdlog::error(
        "{} failure\n\tError code: {}\n\tError message: {}",
        procedure_name,
        strerrorname_np(errno),
        strerror(errno)
    );
}

int main(int argc, char** argv) {
    if(argc < 2) {
        spdlog::error("Program name not specified!");
        return -1;
    }

    const char* program_name = argv[1];
    pid_t pid = fork();

    if(pid == 0) {
        if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
            print_error_message("ptrace");
            return -1;
        }

        if(execl(program_name, program_name, nullptr) == -1) {
            print_error_message("execl");
            return -1;
        }
    } else if(pid >= 1) {
        nkgt::debugger::run(pid);
    } else {
        print_error_message("fork");
        return -1;
    }

    return 0;
}
