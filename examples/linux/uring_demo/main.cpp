#include "context.hpp"

#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <fcntl.h>

#include <cassert>
#include <iostream>
#include <string>
#include <system_error>

using namespace std::string_view_literals;
using namespace execution;

////////////////////////////////////////////////////////////////////////////////

struct file
{
    int _fd;

    explicit file(char const* path)
        : _fd{::open(path, O_WRONLY | O_CREAT, 0644)}
    {
        if (_fd == -1) {
            throw std::system_error{
                std::error_code{errno, std::system_category()},
                "can't open file"
            };
        }
    }

    int operator * ()
    {
        return _fd;
    }

    ~file()
    {
        ::close(_fd);
    }
};

////////////////////////////////////////////////////////////////////////////////

int main()
{
    try {
        file dst {"./hello.txt"};

        uring::context ctx;
        ctx.start(4);

        auto [r] = *this_thread::sync_wait(
            uring::write(ctx, *dst, "hello world!\n"sv)
            | then([] (auto r) {
                std::cout << "written: " << r << "\n";
                return r;
            })
        );

        assert(r == 13);

        ctx.stop();
    }
    catch (std::exception const& ex) {
        std::cerr << "[ERROR] " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
