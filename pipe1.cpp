#include "common.h"

namespace ipcbench {
namespace pipe1 {

class Method : public ipcbench::Method {
public:
    std::string name() const override {
        return "pipe1";
    }

    Method() = default;

    Method(Register) {
        registerMethod(this);
    }

    std::unique_ptr<ipcbench::Method> CreateAnother() const override {
        return std::make_unique<Method>();
    }

    int pipefd[2] = {0};
    size_type sum, n;

    void setup() override {
        buf = allocate_regular();

        errno = 0;
        if (::pipe(pipefd) == -1) {
            throw_errno("pipe");
        }
    }

    void parent() override {
        for (size_type i = 0; i < params._count; i++) {
            // FIXME: handle EAGAIN/EINTR
            errno = 0;
            if (::write(pipefd[1], buf, params._size) != params._size) {
                throw_errno("write");
            }
        }
    }

    void child() override {
        sum = 0;
        for (size_type i = 0; i < params._count; i++) {
            // FIXME: handle EAGAIN/EINTR
            errno = 0;
            n = ::read(pipefd[0], buf, params._size);
            if (n == -1) {
                throw_errno("read");
            }

            sum += n;
        }

        if (sum != params._count * params._size) {
            std::cerr << "sum error: " << sum << " != " << (params._count * params._size) << std::endl;
            throw_runtime("sum error");
        }
    }

} _method{Method::Register{}};

}  // namespace
}  // namespace ipcbench

