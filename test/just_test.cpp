#include "test_receiver.h"

#include <execution/just.h>

#include <gtest/gtest.h>

using namespace NExecution;

////////////////////////////////////////////////////////////////////////////////

TEST(Just, Test)
{
    {
        bool value = false;
        auto op = Connect(Just(), TTestReceiver()
            .OnValue([&] {
                value = true;
            }));

        op.Start();

        EXPECT_EQ(true, value);
    }

    {
        int value = 0;
        auto op = Connect(Just(42), TTestReceiver()
            .OnValue([&] (int x) {
                value = x;
            }));

        op.Start();

        EXPECT_EQ(42, value);
    }

    {
        std::pair<int, int> value {};

        auto op = Connect(Just(1, 2), TTestReceiver()
            .OnValue([&] (int x, int y) {
                value = {x, y};
            }));

        op.Start();

        EXPECT_EQ(1, std::get<0>(value));
        EXPECT_EQ(2, std::get<1>(value));
    }
}
