#include "common.h"

#include <iostream>
#include <list>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace ipcbench;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./main <size> <count> [<method> <method> ...]" << std::endl;
        return 1;
    }

    unsigned long long size = std::atoi(argv[1]);
    unsigned long long count = std::atoi(argv[2]);
    Parameters params{size, count};

    std::list<const IPCMethod*> methods;
    if (argc > 3) {
        for (unsigned int i = 3; i < argc; i++) {
            try {
                methods.push_back(&getMethod(std::string(argv[i])));
            } catch (const std::out_of_range& e) {
                std::cerr << "Unknown method: " << argv[i] << std::endl;
                return 1;
            }
        }
    } else {
        for (const auto& method : allMethods()) {
            methods.push_back(&method.second);
        }
    }

    for (const auto method : methods) {
        method->_setup(params);

        std::cout << method->_name << std::endl;

        auto child_pid = fork();
        if (child_pid == 0) {
            method->_child();
            return 0;
        }

        struct timeval begin, end;
        gettimeofday(&begin, NULL);
        method->_parent();
        gettimeofday(&end, NULL);

        // FIXME
        double tm = getdetlatimeofday(&begin, &end);
        printf("%.0fMB/s %.0fmsg/s\n",
            params._count * params._size * 1.0 / (tm * 1024 * 1024),
            params._count * 1.0 / tm);

        int wstatus;
        waitpid(child_pid, &wstatus, 0);
    }

    return 0;
}
