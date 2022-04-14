#include "common.h"

namespace ipcbench {
namespace pipe {

constexpr auto NAME = "pipe";

struct Method {

    Parameters params;

    int pipefd1[2] = {0};
    int pipefd2[2] = {0};
    char *buf;

    unsigned long long total_read = 0;
    unsigned long long total_write = 0;
    unsigned long long total_mangled = 0;
    unsigned long long total_expected = 0;

    void write_buf(int fd) {
        // FIXME: handle EAGAIN/EINTR
        errno = 0;
        if (::write(fd, buf, params._size) != params._size) {
            perror("write");
        }
    }

    void read_buf(int fd) {
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
    }

    void mangle_buf(size_t n) {
        for (auto i = 0; i < n; i++) {
            size_t r = rand() % params._size;
            buf[r]++;
        }
    }

    void check() {
        if (total_write != total_expected) {
            std::cerr << "total_write error: " << total_write << " != " << total_expected << std::endl;
            throw std::runtime_error("total_write error");
        }
        if (total_read != total_expected) {
            std::cerr << "total_read error: " << total_read << " != " << total_expected << std::endl;
            throw std::runtime_error("total_read error");
        }

        unsigned long long total = 0;
        for (auto i = 0; i < params._size; i++) {
            total += buf[i];
        }
        if (total != 2 * total_mangled) {
            std::cerr << "total error: " << total << " != " << params._count * 10 << std::endl;
            throw std::runtime_error("total error");
        }
    }

    Method() {
        registerMethod(IPCMethod{NAME,

            // Setup
            [&] (const Parameters& p) {
                params = p;

                total_expected = params._count * params._size;

                errno = 0;
                buf = (char*)::malloc(params._size);
                if (buf == NULL) {
                    perror("malloc");
                }
                ::memset(buf, 0, params._size);

                errno = 0;
                if (::pipe(pipefd1) == -1) {
                    perror("pipe");
                }

                errno = 0;
                if (::pipe(pipefd2) == -1) {
                    perror("pipe");
                }
            },

            // Parent
            [&] () {
                for (auto i = 0; i < params._count; i++) {
                    mangle_buf(10);
                    total_mangled += 10;

                    write_buf(pipefd1[1]);
                    total_write += params._size;

                    read_buf(pipefd2[0]);
                    total_read += params._size;
                }

                check();
            },

            // Child
            [&] () {
                for (auto i = 0; i < params._count; i++) {
                    read_buf(pipefd1[0]);
                    total_read += params._size;

                    mangle_buf(10);
                    total_mangled += 10;

                    write_buf(pipefd2[1]);
                    total_write += params._size;
                }

                check();
            },

        });
    }
} _method;

}  // namespace
}  // namespace ipcbench

