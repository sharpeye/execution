#include <execution/when_all.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/schedule.hpp>
#include <execution/simple_thread_pool.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

#include <string>

using namespace std::chrono_literals;

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(when_all, traits)
{
    auto s0 = when_all(
        just(),
        just(1, '2'),
        just(),
        just(3),
        just(4, 'X', 6.0)
    );

    auto s1 = when_all(
        just(),
        just(1) | then([] (auto) { return 1; }),
        just(2) | then([] (auto) {}),
        just(3) | then([] (auto) { return 'X'; }),
        just(4, 5.0)
    );

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto receiver_type = meta::atom<null_receiver>{};

    static_assert(traits::sender_errors(s0_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s1_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<int, char, int, int, char, double>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int, char, int, double>>{});
}

TEST(when_all, simple)
{
    auto s = when_all(
        just(),
        just(1, '2'),
        just(),
        just(3),
        just(4, 'X', 6.0)
    );

    auto [r0, r1, r2, r3, r4, r5] = *this_thread::sync_wait(std::move(s));

    EXPECT_EQ(1, r0);
    EXPECT_EQ('2', r1);
    EXPECT_EQ(3, r2);
    EXPECT_EQ(4, r3);
    EXPECT_EQ('X', r4);
    EXPECT_EQ(6.0, r5);
}

TEST(when_all, thread_pool)
{
    simple_thread_pool pool{3};

    auto sched = pool.get_scheduler();

    auto s = when_all(
        schedule(sched) | then([] { return 42; }),
        schedule_after(sched, 100ms) | then([]{ return 'X'; }),
        schedule_after(sched, 50ms) | then([]{ return 100.0; })
    );

    auto [r0, r1, r2] = *this_thread::sync_wait(std::move(s));

    EXPECT_EQ(42, r0);
    EXPECT_EQ('X', r1);
    EXPECT_EQ(100.0, r2);
}
