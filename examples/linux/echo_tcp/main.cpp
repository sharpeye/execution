#include <execution/conditional.hpp>
#include <execution/finally.hpp>
#include <execution/just.hpp>
#include <execution/let_value.hpp>
#include <execution/op.hpp>
#include <execution/repeat_effect_until.hpp>
#include <execution/sequence.hpp>
#include <execution/start_detached.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>
#include <execution/thread_pool.hpp>
#include <execution/upon_error.hpp>
#include <execution/linux/io_uring.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>

#include <array>
#include <cassert>
#include <iostream>
#include <string>
#include <system_error>

using namespace execution;
namespace io = execution::io_uring;

namespace {

////////////////////////////////////////////////////////////////////////////////

/*std::string to_string(sockaddr_in const& peer)
{
    char addr[INET_ADDRSTRLEN] {};
    inet_ntop(AF_INET, &peer.sin_addr, addr, sizeof(addr));

    return std::string{addr} + ":" + std::to_string(peer.sin_port);
}*/

std::string to_string(sockaddr_in6 const& peer)
{
    char addr[INET6_ADDRSTRLEN] {};
    inet_ntop(AF_INET6, &peer.sin6_addr, addr, sizeof(addr));

    return std::string{addr} + ":" + std::to_string(peer.sin6_port);
}

void print_error(std::exception_ptr ep)
{
    try {
        std::rethrow_exception(ep);
    } catch (std::exception const& ex) {
        std::cerr << "[ERROR] " << ex.what() << '\n';
    }
}

void print_error(std::error_code ec)
{
    std::cerr << "[ERROR] " << ec << '\n';
}

void fatal(char const* msg, int ec = errno)
{
    throw std::system_error{ec, std::system_category(), msg};
}

int bind_and_listen(int port)
{
    int const s = socket(AF_INET6, SOCK_STREAM, 0);
    int const val = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    sockaddr_in6 addr {
        .sin6_family = AF_INET6,
        .sin6_port = htons(port),
        .sin6_addr = in6addr_any
    };

    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        fatal("bind");
    }

    if (listen(s, 5) < 0) {
        fatal("listen");
    }

    return s;
}

////////////////////////////////////////////////////////////////////////////////

struct connection
{
    int _fd = -1;

    std::array<std::byte, 64 * 1024> _buffer;
    bool _done = false;

    auto process(io::context& ctx)
    {
        static constexpr auto prefix = std::span{ ">> ", 3 };
        static constexpr auto bye = std::span{ "bye", 3 };

        return io::read_some(ctx, _fd, _buffer)
            | let_value([&ctx, this] (std::span<std::byte> buf) {
                _done = buf.empty();
                return conditional([this] { return _done; },
                    io::write(ctx, _fd, as_bytes(bye)),
                    sequence(
                        io::write(ctx, _fd, as_bytes(prefix)),
                        io::write(ctx, _fd, buf)
                    ));
            })
            | repeat_effect_until([this] { return _done; })
            | upon_error([] (auto error) { print_error(error); })
            | finally(io::close(ctx, _fd));
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    static std::stop_source stop_source;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  [] (int) { stop_source.request_stop(); });
    signal(SIGTERM, [] (int) { stop_source.request_stop(); });

    try {
        int const port = argc > 1
            ? std::stoi(argv[1])
            : 9788;

        int const s = bind_and_listen(port);

        std::clog << "listening port " << port << " ..." << '\n';

        io::context ctx;

        std::thread loop {[&] {
            ctx.run();
        }};

        on(ctx.get_scheduler(), io::accept(ctx, s)
            | then([&] (int s, sockaddr_in6 const& peer) {
                std::clog << "new connection: " << to_string(peer) << '\n';

                start_detached(just(connection {s})
                    | let_value([&] (connection& conn) {
                        return conn.process(ctx);
                    })
                    | finally(just(peer) | then([] (sockaddr_in6 const& peer) {
                        std::clog << "done with " << to_string(peer) << '\n';
                    }))
                );
              })
            | repeat_effect()
            | upon_error([] (auto error) { print_error(error); })
            | finally(io::close(ctx, s) | then([&] {
                ctx.shutdown();
            })))
            | this_thread::sync_wait(stop_source);

        loop.join();
    }
    catch (...) {
        print_error(std::current_exception());
        return 1;
    }

    return 0;
}
