#include "common.h"

#include <iostream>
#include <list>
#include <unistd.h>

using namespace ipcbench;

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: ./main <size> <target_runtime_us> <num_mangle> [<method> <method> ...]" << std::endl;
        return 1;
    }

    size_type size = std::atoll(argv[1]);
    size_type target_runtime_us = std::atoll(argv[2]);
    size_type num_mangle = std::atoll(argv[3]);

    if (size % 4 != 0) {
        size_type new_size = (size / 4 + 1) * 4;
        std::cerr << "Warning: size of " << size << " is not a multiple of 4, rounding up to " << new_size << std::endl;
        size = new_size;
    }

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

    std::cout << "main size " << size << " target_runtime_us " << target_runtime_us << " num_mangle " << num_mangle;
    for (auto method : methods) {
        std::cout << " " << method->name();
    }
    std::cout << std::endl;

    Parameters test_params{size, 10, num_mangle};
    int exitcode = 0;
    for (auto method : methods) {
        try {
            auto other = method->CreateAnother();

            std::cout << "(" << method->name() << " size " << test_params._size << " count " << test_params._count << " num_mangle " << test_params._num_mangle << ")" << std::endl;
            other->init(test_params);
            other->setup();
            other->pre_execute();
            other->execute();

            // If it didn't run successfully, no point in trying to run it for real
            other->results.rethrowExceptions();

            other->results.updateDerivedFields();

            auto diff_us = other->results.diff_us;
            if (diff_us == 0) {
                diff_us = 1;
            }
            size_type count = target_runtime_us * test_params._count / diff_us;
            if (count == 0) {
                count = 1;
            }
            std::cout << test_params._count << " iterations took " << other->results.diff_us << " us, so running " << count << " iterations" << std::endl;

            Parameters params{size, count, num_mangle};

            std::cout << method->name() << " size " << params._size << " count " << params._count << " num_mangle " << params._num_mangle << std::endl;
            method->init(params);
            method->setup();
            method->pre_execute();
            method->execute();

            method->results.humanOutput(std::cout);
            method->results.outputStatsFile("stats");

            method->results.rethrowExceptions();

        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            exitcode++;
        } catch (...) {
            std::cerr << "Unknown exception" << std::endl;
            exitcode++;
        }
        std::cout << std::endl;
    }

    return exitcode;
}
