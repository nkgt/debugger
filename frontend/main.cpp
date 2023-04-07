#include <filesystem>
namespace fs = std::filesystem;

#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <unistd.h>

#include <fmt/core.h>
#include <fmt/std.h>

#include "nkgt/debugger.hpp"
#include "nkgt/util.hpp"

static void execute_debugee(const char* program_name) {
    if(personality(ADDR_NO_RANDOMIZE) == -1) {
        nkgt::util::print_error_message("personality", errno);
        std::exit(EXIT_FAILURE);
    }

    if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
        nkgt::util::print_error_message("ptrace", errno);
        std::exit(EXIT_FAILURE);
    }

    if(execl(program_name, program_name, nullptr) == -1) {
        nkgt::util::print_error_message("execl", errno);
        std::exit(EXIT_FAILURE);
    }
}

int main(int argc, const char** argv) {
    if(argc < 2) {
        fmt::print("Program name not specified!");
        return -1;
    }

    const fs::path program_path(argv[1]);
    if(!fs::exists(program_path) || !fs::is_regular_file(program_path)) {
        fmt::print("The file {} does not exists or is not a regular file.\n", program_path);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if(pid == 0) {
        execute_debugee(program_path.c_str());
    } else if(pid >= 1) {
        nkgt::debugger::run(pid, program_path);
    } else {
        nkgt::util::print_error_message("fork", errno);
        return -1;
    }

    return EXIT_SUCCESS;
}
