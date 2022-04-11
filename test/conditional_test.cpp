#include <execution/conditional.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(just, traits)
{
    auto s0 = conditional([]{ return true; }, just(1), just(1.0));
    auto s1 = conditional([]{ return true; }, just(1), just());
    auto s2 = conditional([]{ return true; }, just(), just());

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
        == meta::list<signature<int>, signature<double>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int>, signature<>>{});

    static_assert(traits::sender_values(s2_type, receiver_type)
        == meta::list<signature<>>{});
}

TEST(just, test)
{
    {
        auto [r0, r1] = *this_thread::sync_wait(conditional(
            []{ return true; },
            just(1, 2),
            just(3, 4)
        ));
        EXPECT_EQ(1, r0);
        EXPECT_EQ(2, r1);
    }

    {
        auto [r0, r1] = *this_thread::sync_wait(conditional(
            []{ return false; },
            just(1, 2),
            just(3, 4)
        ));
        EXPECT_EQ(3, r0);
        EXPECT_EQ(4, r1);
    }
}
