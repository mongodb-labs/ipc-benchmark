#include "common.h"

namespace ipcbench {
namespace pipe1 {

constexpr auto NAME = "pipe1";

struct Method {

    Parameters params;

    int pipefd[2] = {0};
    unsigned long long sum, n;
    char *buf;

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
                if (::pipe(pipefd) == -1) {
                    perror("pipe");
                }
            },

            // Parent
            [&] () {
                for (auto i = 0; i < params._count; i++) {
                    // FIXME: handle EAGAIN/EINTR
                    errno = 0;
                    if (::write(pipefd[1], buf, params._size) != params._size) {
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
                    n = ::read(pipefd[0], buf, params._size);
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

