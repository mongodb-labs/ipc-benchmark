#include "common.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace ipcbench {
namespace shm {

class Method : public ipcbench::Method {
public:
    Method() {
        registerMethod(this);
    }

    std::string name() const override {
        return "shm";
    }

	int segment_id;

	int guard_id;


    void setup() override {
        errno = 0;
        key_t segment_key = ftok("main", 'X');
        if (segment_key < 0) {
            throw_errno("ftok");
        }

        errno = 0;
        segment_id = shmget(segment_key, params._size, IPC_CREAT | 0666);
        if (segment_id < 0) {
            throw_errno("shmget");
        }

        errno = 0;
        char* addr = (char*)shmat(segment_id, NULL, 0);
        if (addr < (char*)0) {
            throw_errno("shmat");
        }
        buf = (unsigned char*) addr;

        zero_buf();


        errno = 0;
        key_t guard_key = ftok("main", 'G');
        if (guard_key < 0) {
            throw_errno("guard ftok");
        }

        errno = 0;
        // Having the guard as the first naked byte will likely cause cache line chaos for small bufs.
        // It should be in a separate shmem segment.
        guard_id = shmget(guard_key, 1, IPC_CREAT | 0666);
        if (guard_id < 0) {
            throw_errno("guard shmget");
        }

        errno = 0;
        guard = static_cast<std::atomic<unsigned char>*>(shmat(guard_id, NULL, 0));
        if (guard < 0) {
            throw_errno("guard shmat");
        }

        guard->store(2);
    }

    void parent() override {

        wait_for_init();

        for (size_type i = 0; i < params._count; i++) {
            mangle_buf();
            give_control_to_child();

            wait_for_parent_control();
        }
    }

    void parent_finish() override {
        check_total_mangled();

        errno = 0;
        if (shmdt(buf) < 0) {
            throw_errno("shmdt");
        }

        errno = 0;
        if (shmctl(segment_id, IPC_RMID, NULL) < 0) {
            throw_errno("shmctl rmid");
        }

        errno = 0;
        if (shmdt(guard) < 0) {
            throw_errno("guard shmdt");
        }

        errno = 0;
        if (shmctl(guard_id, IPC_RMID, NULL) < 0) {
            throw_errno("guard shmctl rmid");
        }
    }

    void child() override {

        wait_for_init();

        for (size_type i = 0; i < params._count; i++) {
            wait_for_child_control();

            mangle_buf();
            give_control_to_parent();
        }
    }

    void child_finish() override {
        check_total_mangled();

        errno = 0;
        if (shmdt(buf) < 0) {
            throw_errno("shmdt");
        }

        errno = 0;
        if (shmdt(guard) < 0) {
            throw_errno("guard shmdt");
        }
    }

} _method;

}  // namespace
}  // namespace ipcbench

