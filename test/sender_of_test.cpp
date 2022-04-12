#include <execution/sender_of.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(sender_of, traits)
{
    struct foo {};

    auto s0 = sender_of<>(just());
    auto s1 = sender_of<int>(just(1));
    auto s2 = sender_of<foo>(just(foo{}));

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
        == meta::list<signature<foo>>{});
}

TEST(sender_of, test)
{
    {
        auto r = this_thread::sync_wait(sender_of<>(just()));
        EXPECT_TRUE(r.has_value());
    }

    {
        auto [r] = *this_thread::sync_wait(sender_of<int>(just(42)));
        EXPECT_EQ(42, r);
    }
}
