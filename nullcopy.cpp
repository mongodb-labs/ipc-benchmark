#include "common.h"

namespace ipcbench {
namespace nullcopy {

class Method : public ipcbench::Method {
public:
    Method() {
        registerMethod(this);
    }

    std::string name() const {
        return "nullcopy";
    }

    unsigned char* parent_buf;
    unsigned char* child_buf;

    void setup() {
        buf = allocate_regular();
        zero_buf();
        parent_buf = buf;

        buf = allocate_regular();
        zero_buf();
        child_buf = buf;
    }

    void pre_execute() override {
        // Don't fork.
    }

    void parent() override {
        for (size_type i = 0; i < params._count; i++) {
            // Mangle as the "parent"
            _isParent = true;
            buf = parent_buf;
            mangle_buf();

            // "Send" to the "child"
            ::memcpy(child_buf, parent_buf, params._size);
            total_write += params._size;

            // Mangle as the "child"
            _isParent = false;
            buf = child_buf;
            mangle_buf();

            // "Receive" from the "child"
            ::memcpy(parent_buf, child_buf, params._size);
            total_read += params._size;
        }
        _isParent = true;
        buf = parent_buf;

        // We're doing all the mangling, so check_total_mangled shouldn't
        // have a factor of 2 for the "other side".  Instead, just halve
        // this value, to cancel out the factor of 2 in check_total_mangled().
        total_mangled = total_mangled / 2;
    }

    void parent_finish() override {
        check_total_read();
        check_total_write();
        check_total_mangled();
    }

    void child() override {
        throw_runtime("nullcopy child");
    }


} _method;

}  // namespace
}  // namespace ipcbench

