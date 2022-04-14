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


struct IPCMethod {
    std::string _name;
    std::function<void(const Parameters&)> _setup;
    std::function<void()> _parent;
    std::function<void()> _child;
};

using IPCMethods = std::map<std::string, IPCMethod>;

void registerMethod(IPCMethod&& method);
const IPCMethod& getMethod(const std::string& name);
const IPCMethods& allMethods();


void perror(const char* what);


double getdetlatimeofday(struct timeval *begin, struct timeval *end);

}  // namespace ipcbench

