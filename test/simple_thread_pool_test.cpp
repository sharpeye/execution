#include <execution/simple_thread_pool.hpp>

#include <execution/schedule.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>

using namespace std::chrono_literals;
using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(simple_thread_pool, simple)
{
    simple_thread_pool pool{2};

    std::promise<int> promise;
    auto future = promise.get_future();

    pool.enqueue([p = std::move(promise)] () mutable {
        p.set_value(42);
    });

    future.wait();
    EXPECT_EQ(42, future.get());
}

TEST(simple_thread_pool, timed)
{
    simple_thread_pool pool{2};

    std::atomic<int> count { 2 };

    std::promise<int> p;
    auto future = p.get_future();

    pool.enqueue(100ms, [&] () mutable {
        if (count.fetch_sub(1) == 1) {
            p.set_value(100);
        }
    });

    pool.enqueue(50ms, [&] () mutable {
        if (count.fetch_sub(1) == 1) {
            p.set_value(50);
        }
    });

    future.wait();
    EXPECT_EQ(100, future.get());
    EXPECT_EQ(0, count.load());
}

TEST(simple_thread_pool, schedule)
{
    simple_thread_pool pool{1};

    auto sched = pool.get_scheduler();

    auto [r] = *this_thread::sync_wait(
        schedule(sched) | then([] { return 42; })
    );

    EXPECT_EQ(42, r);
}
