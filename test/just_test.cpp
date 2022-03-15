#include "test_receiver.hpp"

#include <execution/just.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

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
}
