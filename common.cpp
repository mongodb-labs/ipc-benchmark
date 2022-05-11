#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <fcntl.h>
#include <sys/uio.h>

#include "common.h"

#include <cerrno>
#include <fstream>
#include <iomanip>
#include <limits>
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

    // https://stackoverflow.com/questions/6973995/dynamic-aligned-memory-allocation-in-c11/6974037#6974037
    //
    // This is also interesting because it demonstrates that the `new` operator has a way of
    // constructing an object "in-place" in a `void*` buffer.
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

unsigned char* Method::allocate_mmap(const std::string& name, bool exclusive, size_type size) {
    if (size < 0) {
        size = params._size;
    }

    std::string mmap_filename = "/dev/shm/" + name;
    int flags = O_RDWR | O_CREAT;

    if (exclusive) {
        flags |= O_EXCL;

        // FIXME: use mkstemp() like a big boy
        // Then there will also be no need to call unlink() here.

        errno = 0;
        int res = ::unlink(mmap_filename.c_str());
        if (res < 0 && errno != ENOENT) {  // fine if it's already not there
            throw_errno("unlink");
        }
    }

    errno = 0;
    mmap_fd = ::open(mmap_filename.c_str(), flags, 0600);
    if (mmap_fd < 0) {
        throw_errno("mmap_fd open");
    }

    mmap_filenames.insert(mmap_filename);

    errno = 0;
    int res = ::ftruncate(mmap_fd, size);
    if (res < 0) {
        throw_errno("mmap_fd ftruncate");
    }

    errno = 0;
    void* map = ::mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mmap_fd, 0);
    if (map == MAP_FAILED) {
        throw_errno("mmap_fd mmap");
    }

    return static_cast<unsigned char*>(map);
}

void Method::unlink_mmap_files() {
    for (const auto& mmap_filename : mmap_filenames) {
        errno = 0;
        int res = ::unlink(mmap_filename.c_str());
        if (res < 0) {
            throw_errno("unlink");
        }
    }
}

unsigned char* Method::allocate_mmap_anon(size_type size) {
    if (size < 0) {
        size = params._size;
    }

    errno = 0;
    void* map = ::mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    if (map == MAP_FAILED) {
        throw_errno("anon mmap");
    }

    return static_cast<unsigned char*>(map);
}


unsigned char* Method::allocate_shm(int name, int* id_out, size_type size) {
    if (size < 0) {
        size = params._size;
    }

    errno = 0;
    key_t segment_key = ftok("main", name);
    if (segment_key < 0) {
        throw_errno("ftok");
    }

    errno = 0;
    int segment_id = shmget(segment_key, size, IPC_CREAT | 0666);
    if (segment_id < 0) {
        throw_errno("shmget");
    }

    errno = 0;
    char* addr = (char*)shmat(segment_id, NULL, 0);
    if (addr < (char*)0) {
        throw_errno("shmat");
    }

    *id_out = segment_id;
    return (unsigned char*) addr;
}

void Method::detach_shm(void* mem) {
    errno = 0;
    if (shmdt(mem) < 0) {
        throw_errno("shmdt");
    }
}

void Method::remove_shm(int id) {
    errno = 0;
    if (shmctl(id, IPC_RMID, NULL) < 0) {
        throw_errno("shmctl rmid");
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
    results.total_read += params._size;
}

void Method::check_total_read() {
    results.updateDerivedFields();
    if (results.total_read != results.total_expected) {
        std::cerr << "total_read error: " << results.total_read << " != " << results.total_expected << std::endl;
        throw_runtime("total_read error");
    }
}


void Method::write_buf(int fd) {
    // FIXME: handle EAGAIN/EINTR
    errno = 0;
    if (::write(fd, buf, params._size) != params._size) {
        throw_errno("write");
    }
    results.total_write += params._size;
}

void Method::check_total_write() {
    results.updateDerivedFields();
    if (results.total_write != results.total_expected) {
        std::cerr << "total_write error: " << results.total_write << " != " << results.total_expected << std::endl;
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
    results.total_write += params._size;
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
    results.total_read += params._size;

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
    results.total_mangled += n;
}

void Method::check_total_mangled() {
    if (params._num_mangle * params._count > 255 * params._size) {
        std::cerr << "warning: num_mangle * count > 255 * size, so mangling counts cannot be verified" << std::endl;
        results.checked_mangled = false;
        return;
    }

    results.checked_mangled = true;

    results.total_mangled_sum = 0;
    for (size_type i = 0; i < params._size; i++) {
        results.total_mangled_sum += buf[i];
    }
    if (results.total_mangled_sum != 2 * results.total_mangled) {
        std::cerr << "total mangled error: " << results.total_mangled_sum << " != " << 2 * results.total_mangled << std::endl;
        throw_runtime("total mangled error");
    }
}


void Method::give_to(std::atomic<unsigned char>* g, char target) {
    g->store(target);
}

void Method::wait_for(std::atomic<unsigned char>* g, char target) {
    while (g->load() != target) {
        // spinwait
    }
}

void Method::give_control_to_parent() {
    give_to(guard, _PARENT);
}

void Method::give_control_to_child() {
    give_to(guard, _CHILD);
}

void Method::wait_for_parent_control() {
    wait_for(guard, _PARENT);
}

void Method::wait_for_child_control() {
    wait_for(guard, _CHILD);
}


void Method::wait_for_init() {
    //std::cerr << (isParent() ? "parent" : "child") << " waiting for init..." << std::endl;
    (*initguard)--;
    wait_for(initguard, 0);
    //std::cerr << (isParent() ? "parent" : "child") << " finished waiting for init" << std::endl;
}


void Method::pre_execute() {
    initguard = static_cast<std::atomic<unsigned char>*>((void*)allocate_mmap_anon(pagesize()));
    initguard->store(2);

    errno = 0;
    _child_pid = fork();
    if (_child_pid < 0) {
        throw_errno("fork");
    }

    _isParent = _child_pid != 0;
}

void Method::init(const Parameters& p) {
    // FIXME: this can go away, if all occurences of `params` are changed to `results.params`...
    params = p;

    results.params = p;
}

void ExecutionResults::updateDerivedFields() {
    total_expected = params._count * params._size;

    timersub(&end, &begin, &diff);

    begin_us = begin.tv_sec * 1000000 + begin.tv_usec;
    end_us = end.tv_sec * 1000000 + end.tv_usec;
    diff_us = diff.tv_sec * 1000000 + diff.tv_usec;

    mb = params._count * params._size * 1.0 / 1048576;
    mb_sec = (diff_us != 0 ? mb * 1000000 / diff_us : std::numeric_limits<double>::infinity());

    msgs_sec = (diff_us != 0 ? params._count * 1000000 / diff_us : std::numeric_limits<double>::infinity());
}

void ExecutionResults::humanOutput(std::ostream& out) {
    updateDerivedFields();

    out << std::fixed << std::setprecision(3);
    out << diff_us << " us  ";
    out << mb_sec << " MB/s  ";
    out << msgs_sec << " msgs/s  ";
    out << std::endl;
}

void ExecutionResults::outputStatsFile(std::string fname) {
    updateDerivedFields();

    std::ofstream stats(fname, std::ios_base::app);

    stats << "name " << name;
    stats << "\tsize " << params._size;
    stats << "\tcount " << params._count;
    stats << "\tnum_mangle " << params._num_mangle;

    stats << "\tbegin_us " << begin_us;
    stats << "\tend_us " << end_us;
    stats << "\tdiff_us " << diff_us;

    stats << "\teptr " << static_cast<bool>(eptr);

    stats << "\tmb " << mb;
    stats << "\tmb_sec " << mb_sec;
    stats << "\tmsgs_sec " << msgs_sec;

    stats << "\ttotal_expected " << total_expected;
    stats << "\ttotal_read " << total_read;
    stats << "\ttotal_write " << total_write;

    stats << "\tchecked_mangled " << checked_mangled;
    stats << "\ttotal_mangled " << total_mangled;
    stats << "\t2*total_mangled " << (2*total_mangled);
    stats << "\ttotal_mangled_sum " << total_mangled_sum;

    stats << std::endl;
}

void Method::execute() {
    if ( ! isParent()) {
        child_setup();
        wait_for_init();
        child();
        child_finish();
        exit(0);
    }

    parent_setup();

    wait_for_init();

    gettimeofday(&results.begin, NULL);
    parent();
    gettimeofday(&results.end, NULL);

    results.updateDerivedFields();

    try {
        parent_finish();
    } catch (...) {
        results.eptr = std::current_exception();
    }

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

