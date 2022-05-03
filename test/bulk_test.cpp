#include <execution/bulk.hpp>
#include <execution/just.hpp>

#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(bulk, traits)
{
    struct foo {};

    auto s0 = just() | bulk(1, [] (int) {});
    auto s1 = just(1) | bulk(1, [] (int, int) {});
    auto s2 = just(std::string{}, 1.0, foo{}) | bulk(1, [] (auto...) {});

    constexpr auto s0_type = meta::atom<decltype(s0)>{};
    constexpr auto s1_type = meta::atom<decltype(s1)>{};
    constexpr auto s2_type = meta::atom<decltype(s2)>{};
    constexpr auto receiver_type = meta::atom<null_receiver>{};

    static_assert(traits::sender_errors(s0_type, receiver_type)
        == meta::list<std::exception_ptr>{});
    static_assert(traits::sender_errors(s1_type, receiver_type)
        == meta::list<std::exception_ptr>{});
    static_assert(traits::sender_errors(s2_type, receiver_type)
        == meta::list<std::exception_ptr>{});

    static_assert(traits::sender_values(s0_type, receiver_type)
        == meta::list<signature<>>{});

    static_assert(traits::sender_values(s1_type, receiver_type)
        == meta::list<signature<int>>{});

    static_assert(traits::sender_values(s2_type, receiver_type)
        == meta::list<signature<std::string, double, foo>>{});
}

TEST(bulk, test)
{
    constexpr size_t shape = 5;
    constexpr size_t len = 9;
    constexpr size_t chunk = (len + 1) / shape;

    auto [r] = *this_thread::sync_wait(
        just(std::vector<int>(len))
            | bulk(shape, [=] (size_t i, std::vector<int>& v) {
                auto it = std::next(v.begin(), i * chunk);
                auto end = std::next(
                    v.begin(),
                    std::min(v.size(), (i + 1) * chunk));

                for (; it != end; ++it) {
                    *it = i;
                }
            }));

    std::vector const expected { 0, 0, 1, 1, 2, 2, 3, 3, 4 };

    EXPECT_EQ(expected, r);
}
