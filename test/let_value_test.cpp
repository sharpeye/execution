#include "test_receiver.h"

#include <execution/just.h>
#include <execution/let_value.h>

#include <gtest/gtest.h>

using namespace NExecution;

////////////////////////////////////////////////////////////////////////////////

TEST(LetValue, Simple)
{
    auto s = Just(1, 2, 3)
        | LetValue([] (int x, int y, int z) {
            return Just(x + y + z);
        });

    int value = 0;

    auto op = Connect(std::move(s), TTestReceiver()
        .OnValue([&] (int x) {
            value = x;
        }));

    op.Start();

    EXPECT_EQ(6, value);
}

TEST(LetValue, Complex)
{
    auto s = Just(1, 2, 3)
        | LetValue([] (int x, int y, int z) {
            return Just(x + y + z);
        })
        | LetValue([] (int x) {
            return Just(x, x)
                | LetValue([] (int x, int y) {
                    return Just(x + y);
                });
        });

    int value = 0;

    auto op = Connect(std::move(s), TTestReceiver()
        .OnValue([&] (int x) {
            value = x;
        }));

    op.Start();

    EXPECT_EQ(12, value);
}
