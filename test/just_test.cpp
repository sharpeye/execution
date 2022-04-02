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
    struct foo {};

    auto s0 = just();
    auto s1 = just(1);
    auto s2 = just(std::string{}, 1.0, foo{});

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

TEST(just, test)
{
    {
        auto r = this_thread::sync_wait(just());
        EXPECT_TRUE(r.has_value());
    }

    {
        auto [r] = *this_thread::sync_wait(just(42));
        EXPECT_EQ(42, r);
    }

    {
        auto [r0, r1] = *this_thread::sync_wait(just(1, 2));
        EXPECT_EQ(1, r0);
        EXPECT_EQ(2, r1);
    }

    {
        std::vector const v0 {1, 2, 3};
        auto s = just(v0);

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
