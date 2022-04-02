#include <execution/let_value.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(let_value, traits)
{
    struct foo { int x; };

    constexpr auto receiver_type = meta::atom<null_receiver>{};

    auto s0 = just() | let_value([] {
        return just();
    });

    auto s1 = just(1, 2, 3)
        | let_value([] (int x, int y, int z) {
            return just(x + y + z);
        });

    auto s2 = just(1, 2, 3)
        | let_value([] (int x, int y, int z) {
            return just(x + y + z);
        })
        | let_value([] (int x) {
            return just(x, foo{x})
                | let_value([] (int x, foo y) {
                    return just(x, y.x);
                });
        });

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto s2_type = meta::atom<decltype(s2)>{};

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int>>{});

    static_assert(traits::sender_values(s2_type, receiver_type)
        == meta::list<signature<int, int>>{});

    static_assert(traits::sender_errors(s0_type, receiver_type)
         == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s1_type, receiver_type)
         == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s2_type, receiver_type)
         == meta::list<std::exception_ptr>{});
}

TEST(let_value, simple)
{
    auto s = just(1, 2, 3)
        | let_value([] (int x, int y, int z) {
            return just(x + y + z);
        });

    int value = 0;

    auto [r] = *this_thread::sync_wait(std::move(s));
    EXPECT_EQ(6, r);
}

TEST(let_value, complex)
{
    auto s = just(1, 2, 3)
        | let_value([] (int x, int y, int z) {
            return just(x + y + z);
        })
        | let_value([] (int x) {
            return just(x, x)
                | let_value([] (int x, int y) {
                    return just(x + y);
                });
        });

    auto [r] = *this_thread::sync_wait(std::move(s));
    EXPECT_EQ(12, r);
}
