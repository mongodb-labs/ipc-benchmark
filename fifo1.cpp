#include "common.h"

namespace ipcbench {
namespace fifo1 {

constexpr auto NAME = "fifo1";

struct Method {

    Parameters params;

    int fd;
    unsigned long long sum, n;
    char *buf;
    const char* filename = ".fifo1-ipc";

    Method() {
        registerMethod(IPCMethod{NAME,

            // Setup
            [&] (const Parameters& p) {
                params = p;

                errno = 0;
                buf = (char*)::malloc(params._size);
                if (buf == NULL) {
                    perror("malloc");
                }

                errno = 0;
                if (::unlink(filename) != 0 && errno != ENOENT) {
                    perror("unlink");
                }
                errno = 0;
                if (::mkfifo(filename, 0700) == -1) {
                    perror("mkfifo");
                }

                errno = 0;
                fd = ::open(filename, O_RDWR);
                if (fd == -1) {
                    perror("open");
                }

                errno = 0;
                if (::unlink(filename) != 0) {
                    perror("unlink");
                }
            },

            // Parent
            [&] () {
                for (auto i = 0; i < params._count; i++) {
                    // FIXME: handle EAGAIN/EINTR
                    errno = 0;
                    if (::write(fd, buf, params._size) != params._size) {
                        perror("write");
                    }
                }
            },

            // Child
            [&] () {
                sum = 0;
                for (auto i = 0; i < params._count; i++) {
                    // FIXME: handle EAGAIN/EINTR
                    errno = 0;
                    n = ::read(fd, buf, params._size);
                    if (n == -1) {
                        perror("read");
                    }

                    sum += n;
                }

                if (sum != params._count * params._size) {
                    std::cerr << "sum error: " << sum << " != " << (params._count * params._size) << std::endl;
                    throw std::runtime_error("sum error");
                }
            },

        });
    }
} _method;

}  // namespace
}  // namespace ipcbench

