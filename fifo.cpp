#include "common.h"

namespace ipcbench {
namespace fifo {

class Method : public ipcbench::Method {
public:
    Method() {
        registerMethod(this);
    }

    std::string name() const override {
        return "fifo";
    }

    int fd;
    size_type sum, n;
    const char* filename = ".fifo-ipc";

    void setup() override {
        allocate_buf();

        errno = 0;
        if (::unlink(filename) != 0 && errno != ENOENT) {
            throw_errno("unlink");
        }
        errno = 0;
        if (::mkfifo(filename, 0700) == -1) {
            throw_errno("mkfifo");
        }

        errno = 0;
        fd = ::open(filename, O_RDWR);
        if (fd == -1) {
            throw_errno("open");
        }

        errno = 0;
        if (::unlink(filename) != 0) {
            throw_errno("unlink");
        }
    }

    void parent() override {
        for (size_type i = 0; i < params._count; i++) {
            // FIXME: handle EAGAIN/EINTR
            errno = 0;
            if (::write(fd, buf, params._size) != params._size) {
                throw_errno("write");
            }
        }
    }

    void child() override {
        sum = 0;
        for (size_type i = 0; i < params._count; i++) {
            // FIXME: handle EAGAIN/EINTR
            errno = 0;
            n = ::read(fd, buf, params._size);
            if (n == -1) {
                throw_errno("read");
            }

            sum += n;
        }

        if (sum != params._count * params._size) {
            std::cerr << "sum error: " << sum << " != " << (params._count * params._size) << std::endl;
            throw std::runtime_error("sum error");
        }
    }

} _method;

}  // namespace
}  // namespace ipcbench

