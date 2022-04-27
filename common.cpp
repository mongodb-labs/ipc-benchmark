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



unsigned char* Method::allocate_regular(size_type size) {
    if (size < 0) {
        size = params._size;
    }

    errno = 0;
    auto mem = (unsigned char*)::malloc(size);
    if (mem == NULL) {
        throw_errno("malloc");
    }

    return mem;
}

int Method::pagesize() {
    static int pagesize = -1;
    if (pagesize < 0) {
        errno = 0;
        pagesize = sysconf(_SC_PAGESIZE);
        if (pagesize < 0) {
            throw_errno("sysconf(_SC_PAGESIZE)");
        }
    }
    return pagesize;
}

unsigned char* Method::allocate_aligned(size_type size) {
    if (size < 0) {
        size = params._size;
    }

    errno = 0;
    auto size_actual = page_multiple(size);
    void* mem;
    int res = ::posix_memalign(&mem, pagesize(), size_actual);
    if (res != 0) {
        throw_errno("posix_memalign", res);
    }

    return static_cast<unsigned char*>(mem);
}

void Method::zero_buf() {
    ::memset(buf, 0, params._size);
}

unsigned char* Method::allocate_mmap(const std::string& name, size_type size) {
    if (size < 0) {
        size = params._size;
    }

    // FIXME: use mkstemp() like a big boy
    // Then there will also be no need to unlink()
    mmap_filename = "/dev/shm/" + name;

    errno = 0;
    int res = ::unlink(mmap_filename.c_str());
    if (res < 0 && errno != ENOENT) {  // fine if it's already not there
        throw_errno("unlink");
    }

    errno = 0;
    mmap_fd = ::open(mmap_filename.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
    if (mmap_fd < 0) {
        throw_errno("mmap_fd open");
    }

    errno = 0;
    res = ::ftruncate(mmap_fd, size);
    if (res < 0) {
        throw_errno("mmap_fd ftruncate");
    }

    errno = 0;
    void* map = ::mmap(NULL, params._size, PROT_READ | PROT_WRITE, MAP_PRIVATE, mmap_fd, 0);
    if (map == MAP_FAILED) {
        throw_errno("mmap_fd mmap");
    }

    return static_cast<unsigned char*>(map);
}

void Method::unlink_mmap_file() {
    errno = 0;
    int res = ::unlink(mmap_filename.c_str());
    if (res < 0) {
        throw_errno("unlink");
    }
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
    size_type size = page_multiple(params._size);

    size_type completed = 0;
    size_type remaining = size;

    while (completed < size) {
        const struct iovec iovec{buf + completed, static_cast<size_t>(remaining)};
        const unsigned long nr_segs = 1;

        errno = 0;
        auto n = vmsplice(fd, &iovec, nr_segs, SPLICE_F_GIFT);
        if (n < 0 && errno != EAGAIN && errno != EINTR) {
            throw_errno("vmsplice");
        }
        completed += n;
        remaining -= n;
    }
    total_write += params._size;
}

void Method::receive_buf_move(int fd, int fd_out) {
    size_type size = page_multiple(params._size);

    // need fd_out to belong to a file mmap'd to buf

    //errno = 0;
    //int res = ::munmap(buf, params._size);
    //if (res < 0) {
    //    throw_errno("munmap");
    //}

    // NOTE: enabling these two ftruncates slows everything down a LOT... why???
    //errno = 0;
    //int res = ::ftruncate(fd_out, 0);
    //if (res < 0) {
    //    throw_errno("mmap_fd ftruncate");
    //}

    //errno = 0;
    //res = ::ftruncate(fd_out, params._size);
    //if (res < 0) {
    //    throw_errno("mmap_fd ftruncate");
    //}

    size_type completed = 0;
    size_type remaining = size;

    while (completed < size) {
        errno = 0;
        loff_t off_out = 0;
        auto n = ::splice(fd, NULL, fd_out, &off_out, size, SPLICE_F_MOVE);
        if (n < 0 && errno != EAGAIN && errno != EINTR) {
            throw_errno("splice");
        }
        completed += n;
        remaining -= n;
    }
    total_read += params._size;

    // maybe try just mremap(2) after, instead of munmap prior and mmap after?
    // doesn't seem to have any/much impact.
    //errno = 0;
    //void* map = ::mremap(buf, params._size, params._size, MREMAP_MAYMOVE);
    //if (map == MAP_FAILED) {
    //    throw_errno("mmap_fd mremap");
    //}
    //buf = static_cast<unsigned char*>(map);

    //errno = 0;
    //void* map = ::mmap(NULL, params._size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd_out, 0);
    //if (map == MAP_FAILED) {
    //    throw_errno("mmap_fd mmap");
    //}
    //buf = static_cast<unsigned char*>(map);
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
        std::cerr << "total mangled error: " << total << " != " << 2 * total_mangled << std::endl;
        throw_runtime("total mangled error");
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



void Method::pre_execute() {
    errno = 0;
    _child_pid = fork();
    if (_child_pid < 0) {
        throw_errno("fork");
    }

    _isParent = _child_pid != 0;
}

void Method::execute() {
    if ( ! isParent()) {
        child_setup();
        // FIXME: sync the parent and child before kicking off
        child();
        child_finish();
        exit(0);
    }

    parent_setup();

    // FIXME: sync the parent and child before kicking off
    // probably best would be to make a pipe pair, and then use
    // close() each side, while the other process is using select()
    // to notice it

    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    parent();
    gettimeofday(&end, NULL);

    parent_finish();

    struct timeval diff;
    timersub(&end, &begin, &diff);

    unsigned long long diff_us = diff.tv_sec * 1000000 + diff.tv_usec;
    std::cout << diff_us << " us  ";

    double mb = params._count * params._size * 1.0 / 1048576;
    double mb_sec = mb * 1000000 / diff_us;

    std::cout << mb_sec << " MB/s  ";

    double msgs_sec = params._count * 1000000 / diff_us;
    std::cout << msgs_sec << " msgs/s  ";

    std::cout << std::endl;

    std::cout << std::endl;

    int wstatus;
    waitpid(_child_pid, &wstatus, 0);
    // FIXME: check if the child failed
    _child_pid = -1;
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

