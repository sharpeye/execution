#include <execution/just_stopped.hpp>

#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(just_stopped, traits)
{
    struct foo {};

    auto s0 = just_stopped();

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto receiver_type = meta::atom<null_receiver>{};

    static_assert(traits::sender_errors(s0_type, receiver_type)
        == meta::list<>{});

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<>>{});
}

TEST(just_stopped, test)
{
    auto r = this_thread::sync_wait(just_stopped());
    EXPECT_FALSE(r.has_value());
}
