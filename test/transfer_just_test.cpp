#include <execution/transfer_just.hpp>

#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>
#include <execution/thread_pool.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(transfer_just, traits)
{
    thread_pool pool {1};

    auto sched = pool.get_scheduler();

    struct foo {};

    auto s0 = transfer_just(sched);
    auto s1 = transfer_just(sched, 1);
    auto s2 = transfer_just(sched, std::string{}, 1.0, foo{});

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto s2_type = meta::atom<decltype(s2)>{};
    constexpr auto receiver_type = meta::atom<null_receiver>{};

    static_assert(traits::sender_errors(s0_type, receiver_type)
        == meta::list<std::exception_ptr>{});
    static_assert(traits::sender_errors(s1_type, receiver_type)
        == meta::list<std::exception_ptr>{});
    static_assert(traits::sender_errors(s2_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int>>{});

    static_assert(traits::sender_values(s2_type, receiver_type)
        == meta::list<signature<std::string, double, foo>>{});
}

TEST(transfer_just, test)
{
    thread_pool pool {1};

    auto sched = pool.get_scheduler();
    auto const main_tid = std::this_thread::get_id();

    {
        auto s = transfer_just(sched, 42);
        EXPECT_EQ(
            sched,
            execution::get_completion_scheduler<execution::set_value_t>(s));
    }

    {
        auto r = this_thread::sync_wait(transfer_just(sched)
            | then([=] {
                EXPECT_NE(main_tid, std::this_thread::get_id());
            }));
        EXPECT_TRUE(r.has_value());
    }

    {
        auto [r] = *this_thread::sync_wait(transfer_just(sched, 42)
            | then([=] (auto x) {
                EXPECT_NE(main_tid, std::this_thread::get_id());
                return x;
            }));
        EXPECT_EQ(42, r);
    }

    {
        auto [r0, r1] = *this_thread::sync_wait(transfer_just(sched, 1, 2));
        EXPECT_EQ(1, r0);
        EXPECT_EQ(2, r1);
    }

    {
        std::vector const v0 {1, 2, 3};
        auto s = transfer_just(sched, v0);

        // copy
        auto [v1] = *this_thread::sync_wait(s);
        EXPECT_EQ(v0, v1);

        // move
        auto [v2] = *this_thread::sync_wait(std::move(s));
        EXPECT_EQ(v0, v2);

        auto [v3] = *this_thread::sync_wait(s);
        EXPECT_TRUE(v3.empty());
    }
}
