#include "common.h"

namespace ipcbench {
namespace pipe {

class Method : public ipcbench::Method {
public:
    Method() {
        registerMethod(this);
    }

    std::string name() const {
        return "pipe";
    }

    int pipefd1[2] = {0};
    int pipefd2[2] = {0};

    void setup() {
        allocate_buf();
        zero_buf();

        errno = 0;
        if (::pipe(pipefd1) == -1) {
            throw_errno("pipe");
        }

        errno = 0;
        if (::pipe(pipefd2) == -1) {
            throw_errno("pipe");
        }
    }

    void parent() {
        for (size_type i = 0; i < params._count; i++) {
            mangle_buf();
            write_buf(pipefd1[1]);

            read_buf(pipefd2[0]);
        }
    }

    void parent_finish() {
        check_total_read();
        check_total_write();
        check_total_mangled();
    }

    void child() {
        for (size_type i = 0; i < params._count; i++) {
            read_buf(pipefd1[0]);

            mangle_buf();
            write_buf(pipefd2[1]);
        }
    }

    void child_finish() {
        check_total_read();
        check_total_write();
        check_total_mangled();
    }

} _method;

}  // namespace
}  // namespace ipcbench

