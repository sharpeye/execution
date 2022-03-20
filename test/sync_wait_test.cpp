#include "test_receiver.hpp"

#include <execution/just.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(sync_wait_r, test)
{
    {
        auto r = just(10)
            | then([] (int v) {
                return v + 1;
            })
            | this_thread::sync_wait_r<int>();

        static_assert(std::is_same_v<std::optional<std::tuple<int>>, decltype(r)>);

        EXPECT_TRUE(r.has_value());
        EXPECT_EQ(11, std::get<0>(*r));
    }

    {
        auto r = just(1, 'X')
            | this_thread::sync_wait_r<int, char>();

        static_assert(std::is_same_v<std::optional<std::tuple<int, char>>, decltype(r)>);

        EXPECT_TRUE(r.has_value());
        EXPECT_EQ(1, std::get<0>(*r));
        EXPECT_EQ('X', std::get<1>(*r));
    }

    {
        auto r = just()
            | this_thread::sync_wait_r<>();

        static_assert(std::is_same_v<std::optional<std::tuple<>>, decltype(r)>);

        EXPECT_TRUE(r.has_value());
    }
}

TEST(sync_wait, test)
{
    {
        auto r = just(10)
            | then([] (int v) {
                return v + 1;
            })
            | this_thread::sync_wait();

        static_assert(std::is_same_v<std::optional<std::tuple<int>>, decltype(r)>);

        EXPECT_TRUE(r.has_value());
        EXPECT_EQ(11, std::get<0>(*r));
    }

    {
        auto r = just(1, 'X')
            | this_thread::sync_wait();

        static_assert(std::is_same_v<std::optional<std::tuple<int, char>>, decltype(r)>);

        EXPECT_TRUE(r.has_value());
        EXPECT_EQ(1, std::get<0>(*r));
        EXPECT_EQ('X', std::get<1>(*r));
    }

    {
        auto r = just()
            | this_thread::sync_wait();

        static_assert(std::is_same_v<std::optional<std::tuple<>>, decltype(r)>);

        EXPECT_TRUE(r.has_value());
    }

    {
        const std::vector v{ 1, 2, 3 };

        auto [r] = *this_thread::sync_wait(just(v));

        EXPECT_EQ((std::vector { 1, 2, 3 }), r);
        EXPECT_EQ(3, r.size());
    }
}
