#include "common.h"

namespace ipcbench {
namespace null {

class Method : public ipcbench::Method {
public:
    Method() {
        registerMethod(this);
    }

    std::string name() const {
        return "null";
    }

    void setup() {
        allocate_buf();
        zero_buf();
    }

    void pre_execute() override {
        // Don't fork.
    }

    void parent() override {
        for (size_type i = 0; i < params._count; i++) {
            // Mangle as the "parent"
            _isParent = true;
            mangle_buf();

            total_write += params._size;

            // Mangle as the "child"
            _isParent = false;
            mangle_buf();

            total_read += params._size;
        }
        _isParent = true;

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
        throw_runtime("null child");
    }


} _method;

}  // namespace
}  // namespace ipcbench
