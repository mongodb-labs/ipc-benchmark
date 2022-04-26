#include "common.h"

namespace ipcbench {
namespace pipesplice {

class Method : public ipcbench::Method {
public:
    Method() {
        registerMethod(this);
    }

    std::string name() const {
        return "pipesplice";
    }

    int pipefd1[2] = {0};
    int pipefd2[2] = {0};

    void setup() {
        allocate_buf_aligned();

        zero_buf();

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
            mangle_buf();

            send_buf_gift(pipefd1[1]);
            // Since we gave away the memory, but will need to receive it
            // normally with read() into a buffer, re-allocate the buffer.
            allocate_buf_aligned();

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

            send_buf_gift(pipefd2[1]);
            // Since we gave away the memory, but will need to receive it
            // normally with read() into a buffer, re-allocate the buffer.
            allocate_buf_aligned();
        }
    }

    void child_finish() {
        check_total_read();
        check_total_write();
        // Since this passes the memory pages themselves back and forth
        // between the processes, the child process doesn't have access
        // to the memory at the end, so it can't check the mangling total.
        //check_total_mangled();
    }

} _method;

}  // namespace
}  // namespace ipcbench

