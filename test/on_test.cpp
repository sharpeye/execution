#include <execution/on.hpp>

#include <execution/just.hpp>
#include <execution/then.hpp>
#include <execution/thread_pool.hpp>

#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(on, traits)
{
    struct foo {};

    thread_pool ctx {1};
    auto sched = ctx.get_scheduler();

    auto s0 = on(sched, just());
    auto s1 = on(sched, just(1));
    auto s2 = on(sched, just(std::string{}, 1.0, foo{}));

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

TEST(on, simple)
{
    thread_pool ctx {1};

    auto sched = ctx.get_scheduler();
    auto const main_tid = std::this_thread::get_id();

    auto s = on(sched, just() | then([] {
        return std::this_thread::get_id();
    }));

    auto [r] = *this_thread::sync_wait(std::move(s));

    EXPECT_NE(main_tid, r);
}
