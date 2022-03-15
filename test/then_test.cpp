#include "test_receiver.hpp"

#include <execution/just.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(then, test)
{
    int value = -1;
    auto s = just()
        | then([]{
            return 1;
        })
        | then([] (int x) {
            return x + x;
        })
        | then([] (int x) {
            return x + 1;
        });

    auto op = connect(std::move(s), test_receiver{}
        .on_value([&] (int x) {
            value = x;
        }));

    op.start();

    EXPECT_EQ(3, value);
}
