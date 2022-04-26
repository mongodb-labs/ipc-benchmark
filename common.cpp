#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <fcntl.h>
#include <sys/uio.h>

#include "common.h"

#include <cerrno>
#include <sstream>

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



void Method::allocate_buf() {
    errno = 0;
    buf = (unsigned char*)::malloc(params._size);
    if (buf == NULL) {
        throw_errno("malloc");
    }
}

namespace {

int pagesize = -1;

}  // namespace

void Method::allocate_buf_aligned() {
    if (pagesize < 0) {
        errno = 0;
        pagesize = sysconf(_SC_PAGESIZE);
        if (pagesize < 0) {
            throw_errno("sysconf(_SC_PAGESIZE)");
        }
    }

    errno = 0;
    int res = ::posix_memalign((void**)&buf, pagesize, params._size);
    if (res != 0) {
        throw_errno("posix_memalign", res);
    }
}

void Method::zero_buf() {
    ::memset(buf, 0, params._size);
}



void Method::read_buf(int fd) {
    size_type completed = 0;
    size_type remaining = params._size;
    while (completed < params._size) {
        errno = 0;
        auto n = ::read(fd, buf + completed, remaining);
        if (n == -1 && errno != EAGAIN && errno != EINTR) {
            throw_errno("read");
        }
        completed += n;
        remaining -= n;
    }
    total_read += params._size;
}

void Method::check_total_read() {
    if (total_read != total_expected) {
        std::cerr << "total_read error: " << total_read << " != " << total_expected << std::endl;
        throw_runtime("total_read error");
    }
}


void Method::write_buf(int fd) {
    // FIXME: handle EAGAIN/EINTR
    errno = 0;
    if (::write(fd, buf, params._size) != params._size) {
        throw_errno("write");
    }
    total_write += params._size;
}

void Method::check_total_write() {
    if (total_write != total_expected) {
        std::cerr << "total_write error: " << total_write << " != " << total_expected << std::endl;
        throw_runtime("total_write error");
    }
}

void Method::send_buf_gift(int fd) {
    errno = 0;
    iovec iovec{buf, static_cast<size_t>(params._size)};
    // FIXME: maybe ensure the size in the iovec is limited to 64k?
    // However, note that the nr_segs arg (the 3rd arg) is limited to
    // IOV_MAX, which is in limits.h and is 1024.
    ssize_t res = vmsplice(fd, &iovec, 1, SPLICE_F_GIFT);
    if (res < 0) {
        throw_errno("vmsplice");
    }
    // FIXME: keep going until res == params._size
    if (res != params._size) {
        std::cerr << "res = " << res << std::endl;
        std::cerr << "params._size = " << params._size << std::endl;
        throw_errno("vmsplice b");
    }
    total_write += params._size;
}

void Method::receive_buf_move(int fd) {
/*
    errno = 0;
    ssize_t res = splice(fd, ???, fd_out, ???, 1, SPLICE_F_MOVE);
    if (res < 0) {
        throw_errno("splice");
    }
    // FIXME: keep going until res == params._size ?
    if (res != params._size) {
        throw_errno("splice b");
    }
    total_read += params._size;
*/
}


void Method::mangle_buf(size_type n) {
    if (n == -1) {
        n = params._num_mangle;
    }
    // ensure no collisions between parent/child manglings
    // parent only does even bytes
    // child only does odd bytes
    size_type adjust = isParent() ? 0 : 1;
    for (size_type i = 0; i < n; i++) {
        size_type r = rand() % params._size;
        r = r / 2 * 2 + adjust;
        buf[r]++;
    }
    total_mangled += n;
}

void Method::check_total_mangled() {
    if (params._num_mangle * params._count > 255 * params._size) {
        std::cerr << "warning: num_mangle * count > 255 * size, so mangling counts cannot be verified" << std::endl;
        return;
    }

    size_type total = 0;
    for (size_type i = 0; i < params._size; i++) {
        total += buf[i];
    }
    if (total != 2 * total_mangled) {
        std::cerr << "total error: " << total << " != " << 2 * total_mangled << std::endl;
        throw_runtime("total error");
    }
}


void Method::give_to(char target) {
    guard->store(target);
}

void Method::wait_for(char target) {
    while (guard->load() != target) {
        // spinwait
    }
}

void Method::wait_for_init() {
    (*guard)--;
    wait_for(0);
}

void Method::give_control_to_parent() {
    give_to(_PARENT);
}

void Method::give_control_to_child() {
    give_to(_CHILD);
}

void Method::wait_for_parent_control() {
    wait_for(_PARENT);
}

void Method::wait_for_child_control() {
    wait_for(_CHILD);
}



void Method::throw_runtime(const char* what) {
    std::ostringstream os;
    os << (isParent() ? "parent: " : "child: ") << what;
    ::ipcbench::throw_runtime(os.str().c_str());
}

void Method::throw_errno(const char* what) {
    throw_errno(what, errno);
}

void Method::throw_errno(const char* what, int _errno) {
    std::ostringstream os;
    os << (isParent() ? "parent: " : "child: ") << what;
    ::ipcbench::throw_errno(os.str().c_str(), _errno);
}



double getdetlatimeofday(struct timeval *begin, struct timeval *end) {
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

void throw_runtime(const char* what) {
    throw std::runtime_error(what);
}

void throw_errno(const char* what) {
    throw_errno(what, errno);
}

void throw_errno(const char* what, int _errno) {
    throw std::system_error(_errno, std::generic_category(), what);
}

}  // namespace ipcbench

