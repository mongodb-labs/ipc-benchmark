#include "common.h"

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
        buf = allocate_shm('X', &segment_id);
        zero_buf();

        guard = static_cast<std::atomic<unsigned char>*>((void*)allocate_shm('G', &guard_id, 1));
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

        detach_shm(buf);
        remove_shm(segment_id);

        detach_shm(guard);
        remove_shm(guard_id);
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

        detach_shm(buf);
        detach_shm(guard);
    }

} _method;

}  // namespace
}  // namespace ipcbench

