#include "nkgt/debugger.hpp"

#include <sstream>
#include <vector>

#include <linenoise.h>
#include <spdlog/spdlog.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> out {};
    std::stringstream ss {s};
    std::string item;

    while(std::getline(ss, item, delimiter)) {
        out.push_back(item);
    }

    return out;
}

static bool is_prefix(const std::string& s, const std::string& of) {
    if(s.size() > of.size()) {
        return false;
    }

    return std::equal(s.cbegin(), s.cend(), of.cbegin());
}

static void continue_execution(pid_t pid) {
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    
    int wait_status = 0;
    int options = 0;
    waitpid(pid, &wait_status, options);
}

static void handle_command(const std::string& line, pid_t pid) {
    std::vector<std::string> args = split(line, ' ');

    if(args.empty()) {
        return;
    }

    std::string& command = args[0];

    if(is_prefix(command, "continue")) {
        continue_execution(pid);
    } else {
        spdlog::error("Unknow command");
    }
}

debugger::debugger(std::string program_name, pid_t pid) :
    _program_name{std::move(program_name)},
    _pid{pid}
{}

void debugger::run() {
    int wait_status = 0;
    int options = 0;

    // wait for the child process to finish launching the programm we want to debug
    waitpid(_pid, &wait_status, options);

    char* line = nullptr;
    while((line = linenoise("dbg> ")) != nullptr) {
        handle_command(line, _pid);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}
