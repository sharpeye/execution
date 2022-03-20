#include <execution/simple_thread_pool.hpp>

#include <gtest/gtest.h>

#include <future>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(simple_thread_pool, test)
{
    simple_thread_pool pool{2};

    std::promise<int> promise;
    auto future = promise.get_future();

    pool.enqueue([p = std::move(promise)] () mutable {
        p.set_value(42);
    });

    future.wait();
    EXPECT_EQ(42, future.get());

    pool.shutdown();
}
