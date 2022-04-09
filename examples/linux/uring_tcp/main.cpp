#include "accept.hpp"
#include "close.hpp"
#include "context.hpp"
#include "read_some.hpp"
#include "write.hpp"

#include <execution/finally.hpp>
#include <execution/just.hpp>
#include <execution/let_value.hpp>
#include <execution/repeat_effect_until.hpp>
#include <execution/submit.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <array>
#include <cassert>
#include <iostream>
#include <string>
#include <system_error>

namespace ex = execution;

namespace {

////////////////////////////////////////////////////////////////////////////////

void fatal(char const* msg, int ec = errno)
{
    throw std::system_error{ec, std::system_category(), msg};
}

struct connection
{
    int _fd;

    std::array<std::byte, 64*1024> _buffer;
    bool _done = false;
};

auto process_connection(uring::context& ctx, int s)
{
    return ex::just(connection{s})
        | ex::let_value([&] (connection& conn) {
            return ex::repeat_effect_until(
                uring::read_some(ctx, conn._fd, conn._buffer)
                    | ex::let_value([&] (std::span<std::byte> buf) {
                        conn._done = buf.empty();
                        return uring::write(ctx, conn._fd, buf);
                    }),
                [&] {
                    return conn._done;
                })
            | ex::finally(uring::close(ctx, conn._fd));
          });
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    try {
        int const port = argc > 1
            ? std::stoi(argv[1])
            : 9788;

        // int s = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        int s = socket(AF_INET, SOCK_STREAM, 0);
        const int val = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

        sockaddr_in addr {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = {
                .s_addr = INADDR_ANY
            }};

        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            fatal("bind");
        }

        if (listen(s, 5) < 0) {
            fatal("listen");
        }

        std::clog << "listening on port :" << port << " ..." << std::endl;

        uring::context ctx;
        ctx.start(1024);

        // TODO: stop_source + SIGINT|SIGTERM

        uring::accept(ctx, s) | ex::then([&] (int s, sockaddr_in peer) {
            char addr[ INET_ADDRSTRLEN ] {};
            inet_ntop(AF_INET, &peer, addr, sizeof(addr));
            std::cout << "new connection: " << addr << std::endl;

            ex::submit(process_connection(ctx, s));
            // ex::submit(ex::just());
        })
        | ex::repeat_effect()
        | this_thread::sync_wait();

        ctx.stop();
    }
    catch (std::exception const& ex) {
        std::cerr << "[ERROR] " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}