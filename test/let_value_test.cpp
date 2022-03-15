#include "test_receiver.hpp"

#include <execution/just.hpp>
#include <execution/let_value.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

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
