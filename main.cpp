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

    std::list<Method*> methods;
    if (argc > 3) {
        for (unsigned int i = 3; i < argc; i++) {
            try {
                methods.push_back(getMethod(std::string(argv[i])));
            } catch (const std::out_of_range& e) {
                std::cerr << "Unknown method: " << argv[i] << std::endl;
                return 1;
            }
        }
    } else {
        for (auto method : allMethods()) {
            methods.push_back(method.second);
        }
    }

    for (auto method : methods) {
        method->init(params);

        std::cout << method->name() << std::endl;

        method->setup();

        auto child_pid = fork();
        if (child_pid == 0) {
            method->_isParent = false;
            method->child_setup();
            // FIXME: sync the parent and child before kicking off
            method->child();
            method->child_finish();
            return 0;
        }

        method->_isParent = true;
        method->parent_setup();

        // FIXME: sync the parent and child before kicking off
        // probably best would be to make a pipe pair, and then use
        // close() each side, while the other process is using select()
        // to notice it

        struct timeval begin, end;
        gettimeofday(&begin, NULL);
        method->parent();
        gettimeofday(&end, NULL);

        method->parent_finish();

        // FIXME
        double tm = getdetlatimeofday(&begin, &end);
        printf("%.0fMB/s %.0fmsg/s\n",
            params._count * params._size * 1.0 / (tm * 1024 * 1024),
            params._count * 1.0 / tm);

        int wstatus;
        waitpid(child_pid, &wstatus, 0);
        // FIXME: check if the child failed
    }

    return 0;
}
