#include "common.h"

namespace ipcbench {
namespace mmapanon {

class Method : public ipcbench::Method {
public:
    Method() {
        registerMethod(this);
    }

    std::string name() const override {
        return "mmapanon";
    }

    void setup() override {
        buf = allocate_mmap_anon();
        // The mmap'd space is guaranteed to be zeros, but zero it all anyway
        // to trigger all the page-faults (ie. actually create all the pages).
        zero_buf();

        guard = static_cast<std::atomic<unsigned char>*>((void*)allocate_mmap_anon(pagesize()));
    }

    void parent() override {
        for (size_type i = 0; i < params._count; i++) {
            mangle_buf();
            give_control_to_child();

            wait_for_parent_control();
        }
    }

    void parent_finish() override {
        check_total_mangled();
    }

    void child() override {
        for (size_type i = 0; i < params._count; i++) {
            wait_for_child_control();

            mangle_buf();
            give_control_to_parent();
        }
    }

    void child_finish() override {
        check_total_mangled();
    }

} _method;

}  // namespace
}  // namespace ipcbench
