#include <execution/linux/io_uring.hpp>

#include <execution/ensure_started.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>
#include <execution/transfer_just.hpp>
#include <execution/upon_stopped.hpp>

#include <chrono>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>

#include <gtest/gtest.h>

using namespace execution;
using namespace std::chrono_literals;
namespace io = execution::io_uring;

////////////////////////////////////////////////////////////////////////////////

TEST(io_uring, schedule)
{
    io::context ctx;

    auto sched = ctx.get_scheduler();
    auto s = sched.schedule()
        | then([&] {
            ctx.shutdown();
            return 42;
        })
        | ensure_started();

    ctx.run();

    auto [r] = *this_thread::sync_wait(std::move(s));
    EXPECT_EQ(42, r);
}

TEST(io_uring, schedule_stopped)
{
    io::context ctx;

    auto sched = ctx.get_scheduler();
    auto s = sched.schedule()
        | then([] { return 1; })
        | upon_stopped([] { return 42; })
        | ensure_started();

    ctx.shutdown();
    ctx.run();

    auto [r] = *this_thread::sync_wait(std::move(s));
    EXPECT_EQ(42, r);
}

TEST(io_uring, schedule_after)
{
    using namespace std::chrono;

    io::context ctx;

    auto sched = ctx.get_scheduler();
    auto s = sched.schedule_after(50ms)
        | then([&] {
            ctx.shutdown();
            return 42;
        })
        | ensure_started();

    auto t0 = steady_clock::now();
    ctx.run();
    auto dt = steady_clock::now() - t0;

    EXPECT_GE(dt, 50ms);
    EXPECT_LT(dt, 150ms);

    auto [r] = *this_thread::sync_wait(std::move(s));
    EXPECT_EQ(42, r);
}

TEST(io_uring, schedule_at)
{
    using namespace std::chrono;

    io::context ctx;

    auto const t0 = steady_clock::now();
    auto const deadline = t0 + 51ms;

    auto sched = ctx.get_scheduler();
    auto s = sched.schedule_at(deadline)
        | then([&] {
            ctx.shutdown();
            return 42;
        })
        | ensure_started();

    ctx.run();
    auto const dt = steady_clock::now() - t0;

    EXPECT_GE(dt, 50ms);
    EXPECT_LT(dt, 150ms);

    auto [r] = *this_thread::sync_wait(std::move(s));
    EXPECT_EQ(42, r);
}

/*
TEST(io_uring, accept)
{
    int const socket = ::socket(AF_INET6, SOCK_STREAM, 0);
    int const val = 1;
    ::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    sockaddr_in6 addr {
        .sin6_family = AF_INET6,
        .sin6_port = htons(9778),
        .sin6_addr = in6addr_any
    };

    EXPECT_GE(0, ::bind(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    EXPECT_GE(::listen(socket, 5), 0);

    io::context ctx;

    auto s = io::accept(ctx, socket)
            | then([&] (int fd) {
                ::close(fd);
                ctx.shutdown();
                return 42;
            })
            /*| upon_stopped([] {
                return 33;
            })*/
            | ensure_started();

    ctx.run();

    auto [r] = *this_thread::sync_wait(std::move(s));
    EXPECT_EQ(42, r);
}
//*/