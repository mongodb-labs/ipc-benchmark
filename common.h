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
#include <sys/wait.h>
#include <unistd.h>

namespace ipcbench {

using size_type = long long;

struct Parameters {
    size_type _size = 0;
    size_type _count = 0;
    size_type _num_mangle = 0;
};


class Method {
public:
    // FIXME: these should be protected
    Parameters params;
    unsigned char* buf;
    std::atomic<unsigned char>* guard;
    pid_t _child_pid = -1;
    bool _isParent = true;


    virtual std::string name() const = 0;

    virtual void setup() {}

    virtual void pre_execute();
    virtual void execute();

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

    int pagesize();

    template <typename N>
    N page_multiple(N n) {
        auto _pagesize = pagesize();
        if (n % _pagesize == 0) {
            // exact multiple of page size
            return n;
        } else {
            return (n / _pagesize + 1) * _pagesize;
        }
    }

protected:
    virtual void allocate_buf();
    virtual void allocate_buf_aligned();
    virtual void zero_buf();


    size_type total_read = 0;
    size_type total_write = 0;
    size_type total_expected = 0;

    virtual void read_buf(int fd);
    virtual void send_buf_gift(int fd);
    virtual void check_total_read();

    virtual void write_buf(int fd);
    virtual void receive_buf_move(int fd);
    virtual void check_total_write();


    size_type total_mangled = 0;

    virtual void mangle_buf(size_type n = -1);
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


    virtual void throw_runtime(const char* what);

    virtual void throw_errno(const char* what);
    virtual void throw_errno(const char* what, int _errno);
};

using Methods = std::map<std::string, Method*>;

void registerMethod(Method* method);
Method* getMethod(const std::string& name);
const Methods& allMethods();


void throw_runtime(const char* what);

void throw_errno(const char* what);
void throw_errno(const char* what, int _errno);


}  // namespace ipcbench

