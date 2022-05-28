#include <execution/ensure_started.hpp>

#include <execution/just.hpp>
#include <execution/schedule.hpp>
#include <execution/then.hpp>
#include <execution/timed_thread_pool.hpp>
#include <execution/upon_stopped.hpp>

#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(ensure_started, traits)
{
    constexpr auto receiver_type = meta::atom<null_receiver>{};

    auto s0 = ensure_started(just());
    auto s1 = ensure_started(just(1, 2, 3));
    auto s2 = ensure_started(just(0.1, 42));

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto s2_type = meta::atom<decltype(s2)>{};

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int, int, int>>{});

    static_assert(traits::sender_values(s2_type, receiver_type)
        == meta::list<signature<double, int>>{});

    static_assert(traits::sender_errors(s0_type, receiver_type)
         == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s1_type, receiver_type)
         == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s2_type, receiver_type)
         == meta::list<std::exception_ptr>{});
}

TEST(ensure_started, test)
{
    timed_thread_pool pool {1};
    auto sched = pool.get_scheduler();

    std::promise<void> promise;
    auto future = promise.get_future();

    auto s = ensure_started(schedule_after(sched, 100ms) | then([&] {
        promise.set_value();
        return 42;
    }));

    future.get();

    auto [r] = *this_thread::sync_wait(std::move(s));
    EXPECT_EQ(42, r);
}

TEST(ensure_started, discard)
{
    timed_thread_pool pool {1};
    auto sched = pool.get_scheduler();

    std::promise<int> promise;
    auto future = promise.get_future();

    ensure_started(schedule_after(sched, 100ms) | upon_stopped([&] {
        promise.set_value(42);
    }));

    EXPECT_EQ(42, future.get());
}

TEST(ensure_started, stopped)
{
    timed_thread_pool pool {1};
    auto sched = pool.get_scheduler();

    std::promise<int> promise;
    auto future = promise.get_future();

    std::stop_source stop_source;
    stop_source.request_stop();

    auto r = this_thread::sync_wait(
        ensure_started(schedule_after(sched, 100ms) | upon_stopped([&] {
            promise.set_value(42);
        })),
        stop_source);

    EXPECT_EQ(42, future.get());
}
