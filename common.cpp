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


void Method::read_buf(int fd) {
    auto completed = 0;
    auto remaining = params._size;
    while (completed < params._size) {
        errno = 0;
        auto n = ::read(fd, buf + completed, remaining);
        if (n == -1 && errno != EAGAIN && errno != EINTR) {
            perror("read");
        }
        completed += n;
        remaining -= n;
    }
    total_read += params._size;
}

void Method::check_total_read() {
    if (total_read != total_expected) {
        std::cerr << "total_read error: " << total_read << " != " << total_expected << std::endl;
        throw std::runtime_error("total_read error");
    }
}


void Method::write_buf(int fd) {
    // FIXME: handle EAGAIN/EINTR
    errno = 0;
    if (::write(fd, buf, params._size) != params._size) {
        perror("write");
    }
    total_write += params._size;
}

void Method::check_total_write() {
    if (total_write != total_expected) {
        std::cerr << "total_write error: " << total_write << " != " << total_expected << std::endl;
        throw std::runtime_error("total_write error");
    }
}


void Method::mangle_buf(size_t n) {
    // ensure no collisions between parent/child manglings
    // parent only does even bytes
    // child only does odd bytes
    size_t adjust = isParent ? 0 : 1;
    for (auto i = 0; i < n; i++) {
        size_t r = rand() % params._size;
        r = r / 2 * 2 + adjust;
        buf[r]++;
    }
    total_mangled += n;
}

void Method::check_total_mangled() {
    unsigned long long total = 0;
    for (auto i = 0; i < params._size; i++) {
        total += buf[i];
    }
    if (total != 2 * total_mangled) {
        std::cerr << "total error: " << total << " != " << 2 * total_mangled << std::endl;
        throw std::runtime_error("total error");
    }
}



double getdetlatimeofday(struct timeval *begin, struct timeval *end) {
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

void perror(const char* what) {
    throw std::system_error(errno, std::generic_category(), what);
}

}  // namespace ipcbench

