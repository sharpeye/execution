#include <execution/finally.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(finally, traits)
{
    struct foo { int x; };

    constexpr auto receiver_type = meta::atom<null_receiver>{};

    auto s0 = finally(just(), just());
    auto s1 = finally(just(1, 2, 3), just(1));
    auto s2 = finally(just(1, 2), just());

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto s2_type = meta::atom<decltype(s2)>{};

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int, int, int>>{});

    static_assert(traits::sender_values(s2_type, receiver_type)
        == meta::list<signature<int, int>>{});

    static_assert(traits::sender_errors(s0_type, receiver_type)
         == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s1_type, receiver_type)
         == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s2_type, receiver_type)
         == meta::list<std::exception_ptr>{});
}

TEST(finally, simple)
{
    bool ok = false;

    auto s = just(1, 2, 3)
        | finally(just() | then([&] {
            ok = true;
        }));

    auto [r0, r1, r2] = *this_thread::sync_wait(std::move(s));
    EXPECT_EQ(1, r0);
    EXPECT_EQ(2, r1);
    EXPECT_EQ(3, r2);
    EXPECT_TRUE(ok);
}

TEST(finally, source_error)
{
    bool ok = false;

    auto s = just(1, 2, 3)
        | then([] (int x, int y, int z) {
            throw std::runtime_error{":("};
        })
        | finally(just() | then([&] {
            ok = true;
        }));

    EXPECT_THROW(*this_thread::sync_wait(std::move(s)), std::runtime_error);
    EXPECT_TRUE(ok);
}

TEST(finally, completion_error)
{
    auto s = just(1, 2, 3)
        | then([] (int x, int y, int z) {
            return x + y + z;
        })
        | finally(just() | then([&] {
            throw std::runtime_error{":("};
        }));

    EXPECT_THROW(*this_thread::sync_wait(std::move(s)), std::runtime_error);
}
