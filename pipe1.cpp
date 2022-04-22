#include "common.h"

namespace ipcbench {
namespace pipe1 {

class Method : public ipcbench::Method {
public:
    Method() {
        registerMethod(this);
    }

    std::string name() const {
        return "pipe1";
    }

    int pipefd[2] = {0};
    unsigned long long sum, n;

    void setup() {
        errno = 0;
        buf = (char*)::malloc(params._size);
        if (buf == NULL) {
            perror("malloc");
        }

        errno = 0;
        if (::pipe(pipefd) == -1) {
            perror("pipe");
        }
    }

    void parent() {
        for (auto i = 0; i < params._count; i++) {
            // FIXME: handle EAGAIN/EINTR
            errno = 0;
            if (::write(pipefd[1], buf, params._size) != params._size) {
                perror("write");
            }
        }
    }

    void child() {
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
    }

} _method;

}  // namespace
}  // namespace ipcbench

