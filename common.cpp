#include "common.h"

#include <cerrno>

namespace ipcbench {

namespace {

Methods _methods;

}  // namespace

void registerMethod(Method* method) {
    _methods.emplace(method->name(), method);
}

Method* getMethod(const std::string& name) {
    return _methods.at(name);
}

const Methods& allMethods() {
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

