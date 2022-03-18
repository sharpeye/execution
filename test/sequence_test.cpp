#include "test_receiver.hpp"

#include <execution/just.hpp>
#include <execution/let_value.hpp>
#include <execution/sequence.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(sequence, traits)
{
    auto s0 = sequence(
        just(),
        just(1) | then([] (auto) {}),
        just(2) | then([] (auto) noexcept {}),
        just(3) | then([] (auto) {}),
        just(4)
    );

    auto s1 = sequence(
        just(),
        just(1) | then([] (auto) noexcept {}),
        just(2) | then([] (auto) noexcept {}),
        just(3) | then([] (auto) noexcept {}),
        just(4)
    );

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto receiver_type = meta::atom<test_receiver<>>{};

    static_assert(traits::sender_errors(s0_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_errors(s1_type, receiver_type)
        == meta::list<>{});

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<int>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int>>{});
}

TEST(sequence, simple)
{
    auto s = sequence(
        just(),
        just(1) | then([] (auto) {}),
        just(2) | then([] (auto) {}),
        just(3) | then([] (auto) {}),
        just(4)
    );

    int value = -1;
    auto op = connect(std::move(s), test_receiver{}
        .on_value([&] (int x) {
            value = x;
        }));

    op.start();

    EXPECT_EQ(4, value);
}

TEST(sequence, complex)
{
    int r0 = -1;
    double r1 = -1;

    auto s = sequence(
        just(1, 2, 3) | let_value([&] (int x, int y, int z) {
            return just(x + y + z) | then([&] (int r) {
                r0 = r;
            });
        }),
        just(7.0) | then([&] (double x) { r1 = x; }),
        just(42.f) | let_value([] (float x) {
            return sequence(
                just(x) | then([] (auto) {}),
                just(x + x) | then([] (auto) {}),
                just(x + x, x) 
                | then([] (float x, float y) {
                    return x + y;
                  })
                | let_value([] (float x) {
                    return just(x + 100.0);
                  })
            );
        }));

    float value = -1;

    auto op = connect(std::move(s), test_receiver{}
        .on_value([&] (int x) {
            value = x;
        }));

    op.start();

    EXPECT_EQ(6, r0);
    EXPECT_EQ(7, r1);
    EXPECT_EQ(226, value);
}
