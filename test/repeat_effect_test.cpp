#include <execution/repeat_effect_until.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(repeat_effect_until, traits)
{
    struct foo {};

    auto s0 = repeat_effect_until(just(), [] {
        return true;
    });

    auto s1 = repeat_effect_until(just(foo{}) | then([] (auto) {}), [] {
        return true;
    });

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto receiver_type = meta::atom<null_receiver>{};

    static_assert(traits::sender_errors(s0_type, receiver_type)
        == meta::list<std::exception_ptr>{});
    static_assert(traits::sender_errors(s1_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<>>{});
}

TEST(repeat_effect_until, loop)
{
    int n = 0;
    int counter = 10;

    auto s =
        repeat_effect_until(
            just() | then([&] {
                ++n;
            }),
            [&] {
                return --counter == 0;
            }
        );

    auto r = this_thread::sync_wait(std::move(s));
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(0, counter);
    EXPECT_EQ(10, n);
}

TEST(repeat_effect, loop)
{
    int counter = 10;

    auto s = just()
        | then([&] {
            if (--counter == 0) {
                throw std::runtime_error{":("};
            }
        })
        | repeat_effect();

    EXPECT_THROW(*this_thread::sync_wait(std::move(s)), std::runtime_error);

    EXPECT_EQ(0, counter);
}
