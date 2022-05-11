#include "common.h"

namespace ipcbench {
namespace null {

class Method : public ipcbench::Method {
public:
    std::string name() const {
        return "null";
    }

    Method() = default;

    Method(Register) {
        registerMethod(this);
    }

    std::unique_ptr<ipcbench::Method> CreateAnother() const override {
        return std::make_unique<Method>();
    }

    std::atomic<unsigned char> initguard_actual;

    void setup() {
        buf = allocate_regular();
        zero_buf();
    }

    void pre_execute() override {
        initguard = &initguard_actual;
        initguard->store(1);

        // Don't fork.
    }

    void parent() override {
        for (size_type i = 0; i < params._count; i++) {
            // Mangle as the "parent"
            _isParent = true;
            mangle_buf();

            results.total_write += params._size;

            // Mangle as the "child"
            _isParent = false;
            mangle_buf();

            results.total_read += params._size;
        }
        _isParent = true;

        // We're doing all the mangling, so check_total_mangled shouldn't
        // have a factor of 2 for the "other side".  Instead, just halve
        // this value, to cancel out the factor of 2 in check_total_mangled().
        results.total_mangled /= 2;
    }

    void parent_finish() override {
        check_total_read();
        check_total_write();
        check_total_mangled();
    }

    void child() override {
        throw_runtime("null child");
    }

} _method{Method::Register{}};

}  // namespace
}  // namespace ipcbench

