#include "test_receiver.h"

#include <execution/just.h>
#include <execution/then.h>

#include <gtest/gtest.h>

using namespace NExecution;

////////////////////////////////////////////////////////////////////////////////

TEST(Then, Test)
{
    int value = -1;
    auto s = Just()
        | Then([]{
            return 1;
        })
        | Then([] (int x) {
            return x + x;
        })
        | Then([] (int x) {
            return x + 1;
        });

    auto op = Connect(std::move(s), TTestReceiver()
        .OnValue([&] (int x) {
            value = x;
        }));

    op.Start();

    EXPECT_EQ(3, value);
}
