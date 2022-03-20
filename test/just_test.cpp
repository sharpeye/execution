#include "test_receiver.hpp"

#include <execution/just.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

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
    constexpr auto receiver_type = meta::atom<test_receiver<>>{};

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
        bool value = false;
        auto op = connect(just(), test_receiver{}
            .on_value([&] {
                value = true;
            }));

        op.start();

        EXPECT_EQ(true, value);
    }

    {
        int value = 0;
        auto op = connect(just(42), test_receiver{}
            .on_value([&] (int x) {
                value = x;
            }));

        op.start();

        EXPECT_EQ(42, value);
    }

    {
        std::pair<int, int> value {};

        auto op = connect(just(1, 2), test_receiver{}
            .on_value([&] (int x, int y) {
                value = {x, y};
            }));

        op.start();

        EXPECT_EQ(1, std::get<0>(value));
        EXPECT_EQ(2, std::get<1>(value));
    }

    {
        auto s0 = just(std::vector<int>{ 1, 2, 3 });

        auto s1 = then(s0, [] (auto&& v) {
            for (auto& x: v) {
                x *= 2;
            }
            return std::move(v);
        });

        {
            // copy s0
            auto [r] = *this_thread::sync_wait(then(s0, [] (auto&& v) {
                for (auto& x: v) {
                    x *= 2;
                }
                return std::move(v);
            }));

            EXPECT_EQ(3, r.size());
            EXPECT_EQ((std::vector { 2, 4, 6 }), r);
        }

        {
            // move
            auto [r] = *this_thread::sync_wait(then(std::move(s0), [] (auto&& v) {
                for (auto& x: v) {
                    x *= 2;
                }
                return std::move(v);
            }));

            EXPECT_EQ(3, r.size());
            EXPECT_EQ((std::vector { 2, 4, 6 }), r);
        }

        {
            auto [r] = *this_thread::sync_wait(s0);

            EXPECT_EQ(0, r.size());
        }
    }
}
