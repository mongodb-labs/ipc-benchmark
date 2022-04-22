#include "common.h"

#include <atomic>
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

    key_t segment_key;
	int segment_id;

    key_t guard_key;
	int guard_id;
    std::atomic_char* guard;


    static constexpr char _PARENT = 'p';
    static constexpr char _CHILD = 'c';

    void give_to(char target) {
        guard->store(target);
    }

    void wait_for(char target) {
        while (guard->load() != target) {
            // spinwait
        }
    }

    void wait_for_init() {
        (*guard)--;
        wait_for(0);
    }

    void give_control_to_parent() {
        give_to(_PARENT);
    }

    void give_control_to_child() {
        give_to(_CHILD);
    }

    void wait_for_parent_control() {
        wait_for(_PARENT);
    }

    void wait_for_child_control() {
        wait_for(_CHILD);
    }


    void setup() override {
        errno = 0;
        segment_key = ftok("main", 'X');
        if (segment_key < 0) {
            perror("ftok");
        }

        errno = 0;
        segment_id = shmget(segment_key, params._size, IPC_CREAT | 0666);
        if (segment_id < 0) {
            perror("shmget");
        }

        errno = 0;
        buf = (char*)shmat(segment_id, NULL, 0);
        if (buf < (char*)0) {
            perror("shmat");
        }

        ::memset(buf, 0, params._size);


        errno = 0;
        guard_key = ftok("main", 'G');
        if (guard_key < 0) {
            perror("guard ftok");
        }

        errno = 0;
        // Having the guard as the first naked byte will likely cause cache line chaos for small bufs.
        // It should be in a separate shmem segment.
        guard_id = shmget(guard_key, 1, IPC_CREAT | 0666);
        if (guard_id < 0) {
            perror("guard shmget");
        }

        errno = 0;
        guard = static_cast<std::atomic_char*>(shmat(guard_id, NULL, 0));
        if (guard < 0) {
            perror("guard shmat");
        }

        guard->store(2);
    }

    void parent() override {

        wait_for_init();

        for (size_type i = 0; i < params._count; i++) {
            mangle_buf(10);
            give_control_to_child();

            wait_for_parent_control();
        }
    }

    void parent_finish() override {
        check_total_mangled();

        errno = 0;
        if (shmdt(buf) < 0) {
            perror("shmdt");
        }

        errno = 0;
        if (shmctl(segment_id, IPC_RMID, NULL) < 0) {
            perror("shmctl rmid");
        }

        errno = 0;
        if (shmdt(guard) < 0) {
            perror("guard shmdt");
        }

        errno = 0;
        if (shmctl(guard_id, IPC_RMID, NULL) < 0) {
            perror("guard shmctl rmid");
        }
    }

    void child() override {

        wait_for_init();

        for (size_type i = 0; i < params._count; i++) {
            wait_for_child_control();

            mangle_buf(10);
            give_control_to_parent();
        }
    }

    void child_finish() override {
        check_total_mangled();

        errno = 0;
        if (shmdt(buf) < 0) {
            perror("shmdt");
        }

        errno = 0;
        if (shmdt(guard) < 0) {
            perror("guard shmdt");
        }
    }

} _method;

}  // namespace
}  // namespace ipcbench

