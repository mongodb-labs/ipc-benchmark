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
    }

    void parent() {
        for (size_type i = 0; i < params._count; i++) {
            mangle_buf(10);
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

            mangle_buf(10);
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

