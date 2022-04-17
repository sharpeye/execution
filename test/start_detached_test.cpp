#include <execution/start_detached.hpp>

#include <execution/just.hpp>
#include <execution/let_value.hpp>
#include <execution/schedule.hpp>
#include <execution/simple_thread_pool.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

#include <chrono>

using namespace std::chrono_literals;
using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(start_detached, test)
{
    simple_thread_pool pool{1};
    auto sched = pool.get_scheduler();

    std::promise<void> signal;
    std::promise<int> value;

    auto s = signal.get_future();
    auto v = value.get_future();

    start_detached(
        schedule(sched)
            | let_value([&] {
                return just() | then([&] {
                    s.wait();
                    value.set_value(42);
                });
            })
        );

    EXPECT_EQ(std::future_status::timeout, v.wait_for(10ms));
    signal.set_value();
    EXPECT_EQ(42, v.get());
}
