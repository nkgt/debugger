#include <string>

class debugger {
public:
    debugger(std::string program_name, pid_t pid);
    void run();

private:
    std::string _program_name;
    pid_t _pid;
};
