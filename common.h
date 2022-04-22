#include <atomic>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <system_error>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

namespace ipcbench {

using size_type = ssize_t;
struct Parameters {
    size_type _size;
    size_type _count;
};


class Method {
public:
    // FIXME: these should be protected
    Parameters params;
    char *buf;
    std::atomic_char* guard;
    bool _isParent;


    virtual std::string name() const = 0;

    virtual void setup() {}

    virtual void parent_setup() {}
    virtual void child_setup() {}

    virtual void parent() = 0;
    virtual void child() = 0;

    virtual void parent_finish() {}
    virtual void child_finish() {}

    virtual void init(const Parameters& p) {
        params = p;
        total_expected = params._count * params._size;
    }

    bool isParent() const {
        return _isParent;
    }

protected:
    size_type total_read = 0;
    size_type total_write = 0;
    size_type total_expected = 0;

    virtual void read_buf(int fd);
    virtual void check_total_read();

    virtual void write_buf(int fd);
    virtual void check_total_write();


    size_type total_mangled = 0;

    virtual void mangle_buf(size_type n);
    virtual void check_total_mangled();


    static constexpr char _PARENT = 'p';
    static constexpr char _CHILD = 'c';

    virtual void give_to(char target);
    virtual void wait_for(char target);
    virtual void wait_for_init();
    virtual void give_control_to_parent();
    virtual void give_control_to_child();
    virtual void wait_for_parent_control();
    virtual void wait_for_child_control();

};

using Methods = std::map<std::string, Method*>;

void registerMethod(Method* method);
Method* getMethod(const std::string& name);
const Methods& allMethods();


void perror(const char* what);


double getdetlatimeofday(struct timeval *begin, struct timeval *end);

}  // namespace ipcbench

