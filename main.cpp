#include "common.h"

#include <iostream>
#include <list>
#include <unistd.h>

using namespace ipcbench;

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: ./main <size> <count> <num_mangle> [<method> <method> ...]" << std::endl;
        return 1;
    }

    size_type size = std::atoi(argv[1]);
    size_type count = std::atoi(argv[2]);
    size_type num_mangle = std::atoi(argv[3]);
    Parameters params{size, count, num_mangle};

    std::list<Method*> methods;
    if (argc > 4) {
        for (int i = 4; i < argc; i++) {
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

        method->execute();
    }

    return 0;
}
