#include "test_receiver.hpp"

#include <execution/just.hpp>
#include <execution/stop_when.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(stop_when, traits)
{
    auto s0 = stop_when(just(), just());
    auto s1 = stop_when(just(42, 'X'), just(1.1));

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto receiver_type = meta::atom<test_receiver<>>{};

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
