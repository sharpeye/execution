#include "test_receiver.hpp"

#include <execution/just.hpp>
#include <execution/let_value.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(let_value, traits)
{
    struct foo { int x; };

    auto s0 = just(1, 2, 3)
        | let_value([] (int x, int y, int z) {
            return just(x + y + z);
        });

    auto s1 = just(1, 2, 3)
        | let_value([] (int x, int y, int z) {
            return just(x + y + z);
        })
        | let_value([] (int x) {
            return just(x, foo{x})
                | let_value([] (int x, foo y) {
                    return just(x + y.x);
                });
        });

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto receiver_type = meta::atom<test_receiver<>>{};

    static_assert(traits::sender_errors(s0_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s1_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<int>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int>>{});
}

TEST(let_value, simple)
{
    auto s = just(1, 2, 3)
        | let_value([] (int x, int y, int z) {
            return just(x + y + z);
        });

    int value = 0;

    auto op = connect(std::move(s), test_receiver{}
        .on_value([&] (int x) {
            value = x;
        }));

    op.start();

    EXPECT_EQ(6, value);
}

TEST(let_value, complex)
{
    auto s = just(1, 2, 3)
        | let_value([] (int x, int y, int z) {
            return just(x + y + z);
        })
        | let_value([] (int x) {
            return just(x, x)
                | let_value([] (int x, int y) {
                    return just(x + y);
                });
        });

    int value = 0;

    auto op = connect(std::move(s), test_receiver{}
        .on_value([&] (int x) {
            value = x;
        }));

    op.start();

    EXPECT_EQ(12, value);
}
