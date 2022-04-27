#include "common.h"

namespace ipcbench {
namespace pipesplice2 {

class Method : public ipcbench::Method {
public:
    Method() {
        registerMethod(this);
    }

    std::string name() const override {
        return "pipesplice2";
    }

    int pipefd1[2] = {0};
    int pipefd2[2] = {0};

    void setup() override {
        errno = 0;
        if (::pipe(pipefd1) == -1) {
            throw_errno("pipe");
        }

        errno = 0;
        if (::pipe(pipefd2) == -1) {
            throw_errno("pipe");
        }

        // These return the actual size.
        // FIXME: check it
        // Unprivileged max is specified by /proc/sys/fs/pipe-max-size
        ::fcntl(pipefd1[0], F_SETPIPE_SZ, 1048576);
        ::fcntl(pipefd2[0], F_SETPIPE_SZ, 1048576);
    }

    void parent_setup() override {
        buf = allocate_mmap("parent");
    }

    void parent() override {
        for (size_type i = 0; i < params._count; i++) {
            mangle_buf();

            send_buf_gift(pipefd1[1]);
            // Since we gave away the memory, but will need to receive it
            // normally with read() into a buffer, re-allocate the buffer.
            //buf = allocate_aligned();

            //read_buf(pipefd2[0]);
            receive_buf_move(pipefd2[0], mmap_fd);

            //errno = 0;
            //int res = ::munmap(buf, params._size);
            //if (res < 0) {
            //    throw_errno("munmap");
            //}

            //errno = 0;
            //void* map = ::mmap(NULL, params._size, PROT_READ | PROT_WRITE, MAP_PRIVATE, mmap_fd, 0);
            //if (map == MAP_FAILED) {
            //    throw_errno("mmap_fd mmap");
            //}
            //buf = static_cast<unsigned char*>(map);
        }
    }

    void parent_finish() override {
        check_total_read();
        check_total_write();
        check_total_mangled();

        unlink_mmap_file();
    }

    void child_setup() override {
        buf = allocate_mmap("child");
    }

    void child() override {
        for (size_type i = 0; i < params._count; i++) {
            //read_buf(pipefd1[0]);
            receive_buf_move(pipefd1[0], mmap_fd);

            //errno = 0;
            //int res = munmap(buf, params._size);
            //if (res < 0) {
            //    throw_errno("munmap");
            //}

            //errno = 0;
            //void* map = ::mmap(NULL, params._size, PROT_READ | PROT_WRITE, MAP_PRIVATE, mmap_fd, 0);
            //if (map == MAP_FAILED) {
            //    throw_errno("mmap_fd mmap");
            //}
            //buf = static_cast<unsigned char*>(map);

            mangle_buf();

            send_buf_gift(pipefd2[1]);
            // Since we gave away the memory, but will need to receive it
            // normally with read() into a buffer, re-allocate the buffer.
            //buf = allocate_aligned();
        }
    }

    void child_finish() override {
        check_total_read();
        check_total_write();
        // Since this passes the memory pages themselves back and forth
        // between the processes, the child process doesn't have access
        // to the memory at the end, so it can't check the mangling total.
        //check_total_mangled();

        unlink_mmap_file();
    }

} _method;

}  // namespace
}  // namespace ipcbench

