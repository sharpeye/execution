#include <execution/then.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>

#include <gtest/gtest.h>

#include <string>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(then, traits)
{
    struct foo { int x; };

    auto s0 = just() | then([] {});

    auto s1 = just(1) | then([] (auto x) {
        return std::to_string(x);
    });

    auto s2 = just(1) | then([] (auto x) noexcept {
        return foo{x};
    });

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto s2_type = meta::atom<decltype(s2)>{};
    constexpr auto receiver_type = meta::atom<null_receiver>{};

    static_assert(traits::sender_errors(s0_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s1_type, receiver_type)
        // TODO
        // == meta::list<>{});
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s2_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<std::string>>{});

    static_assert(traits::sender_values(s2_type, receiver_type)
        == meta::list<signature<foo>>{});
}

TEST(then, test)
{
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

    auto [r] = *this_thread::sync_wait(std::move(s));

    EXPECT_EQ(3, r);
}
