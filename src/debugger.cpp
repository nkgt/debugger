#include "nkgt/debugger.hpp"
#include "nkgt/util.hpp"

#include <linenoise.h>
#include <spdlog/spdlog.h>

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
        spdlog::error("Unknow command");
    }
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