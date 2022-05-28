#include <execution/thread_pool.hpp>
#include <execution/timed_thread_pool.hpp>

#include <execution/schedule.hpp>
#include <execution/start_detached.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>

using namespace std::chrono_literals;
using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(thread_pool, simple)
{
    thread_pool pool {1};

    auto sched = pool.get_scheduler();

    auto [r] = *this_thread::sync_wait(
        schedule(sched) | then([] { return 42; })
    );

    EXPECT_EQ(42, r);
}

TEST(timed_thread_pool, timed)
{
    timed_thread_pool pool {2};

    std::atomic<int> count {2};

    auto sched = pool.get_scheduler();

    std::promise<int> p;
    auto future = p.get_future();

    start_detached(schedule_after(sched, 100ms) | then([&] () mutable {
        if (count.fetch_sub(1) == 1) {
            p.set_value(100);
        }
    }));

    start_detached(schedule_after(sched, 50ms) | then([&] () mutable {
        if (count.fetch_sub(1) == 1) {
            p.set_value(50);
        }
    }));

    future.wait();
    EXPECT_EQ(100, future.get());
    EXPECT_EQ(0, count.load());
}
