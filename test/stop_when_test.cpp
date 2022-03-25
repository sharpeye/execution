#include <execution/stop_when.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/schedule.hpp>
#include <execution/simple_thread_pool.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

#include <chrono>

using namespace std::chrono_literals;

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(stop_when, traits)
{
    auto s0 = stop_when(just(), just());
    auto s1 = stop_when(just(42, 'X'), just(1.1));

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto receiver_type = meta::atom<null_receiver>{};

    static_assert(traits::sender_errors(s0_type, receiver_type)
        == meta::list<std::exception_ptr>{});
    static_assert(traits::sender_errors(s1_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<>>{});
    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int, char>>{});
}

TEST(stop_when, simple)
{
    auto [r] = *this_thread::sync_wait(
          just(42)
        | stop_when(just())
    );

    EXPECT_EQ(42, r);
}

TEST(stop_when, stopped)
{
    simple_thread_pool pool{2};
    auto sched = pool.get_scheduler();

    auto r = this_thread::sync_wait(
        stop_when(
            schedule_after(sched, 100ms) | then([] {
                return 0;
            }),
            just(42)
        )
    );

    EXPECT_FALSE(r.has_value());
}
