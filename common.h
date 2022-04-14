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

struct Parameters {
    std::size_t _size;
    unsigned long long _count;
};


class Method {
public:
    Parameters params;

    virtual void init(const Parameters& p) {
        params = p;
    }

    virtual std::string name() const = 0;

    virtual void setup() {}

    virtual void parent_setup() {}
    virtual void child_setup() {}

    virtual void parent() = 0;
    virtual void child() = 0;

    virtual void parent_check() {}
    virtual void child_check() {}
};

using Methods = std::map<std::string, Method*>;

void registerMethod(Method* method);
Method* getMethod(const std::string& name);
const Methods& allMethods();


void perror(const char* what);


double getdetlatimeofday(struct timeval *begin, struct timeval *end);

}  // namespace ipcbench

