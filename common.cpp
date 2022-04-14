#include "common.h"

#include <cerrno>

namespace ipcbench {

namespace {

IPCMethods _methods;

}  // namespace

void registerMethod(IPCMethod&& method) {
    _methods.emplace(method._name, std::move(method));
}

const IPCMethod& getMethod(const std::string& name) {
    return _methods.at(name);
}

const IPCMethods& allMethods() {
    return _methods;
}

double getdetlatimeofday(struct timeval *begin, struct timeval *end) {
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

void perror(const char* what) {
    throw std::system_error(errno, std::generic_category(), what);
}

}  // namespace ipcbench

