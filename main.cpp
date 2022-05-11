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

    size_type test_count = 2;
    Parameters test_params = params;
    test_params._count = test_count;

    for (auto method : methods) {
        auto other = method->CreateAnother();

        other->init(test_params);


        std::cout << method->name() << std::endl;
        method->init(params);
        method->setup();
        method->pre_execute();
        method->execute();

        method->results.humanOutput(std::cout);
        std::cout << std::endl;

        method->results.outputStatsFile("stats");

        // feh - just print it, and keep going
        // and return non-zero when exiting the program
        //if (method->results.eptr) {
        //    std::rethrow_exception(method->results.eptr);
        //}

    }

    return 0;
}
