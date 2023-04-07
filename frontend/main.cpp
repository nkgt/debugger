#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>
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

// Basic check for the input file validity.
// Just for safety, we accept only ELF executable as debugee. As per elf(3), a
// ELF executable will start with a magic nubmer formed by the bytes 0x7f, 'E',
// 'L', 'F'. So we just load the first 4 bytes and check them.
static bool is_file_valid(const fs::path& program_path) {
    const bool condition = fs::exists(program_path) && 
                           fs::is_regular_file(program_path) &&
                           fs::file_size(program_path) >= 4;

    if(condition) {
        std::ifstream executable(program_path);

        if(!executable) {
            fmt::print("Failed to open file {}.\n", program_path);
            return false;
        }

        char magic_number[4];
        executable.read(magic_number, 4);
        executable.close();

        return magic_number[0] == 0x7f &&
               magic_number[1] == 'E'  &&
               magic_number[2] == 'L'  &&
               magic_number[3] == 'F';
    }

    return false;
}

int main(int argc, const char** argv) {
    if(argc < 2) {
        fmt::print("Program name not specified!");
        return -1;
    }

    const fs::path program_path(argv[1]);
    if(!is_file_valid(program_path)) {
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
