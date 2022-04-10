#include <execution/upon_error.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

#include <string>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(upon_error, traits)
{
    auto s0 = just() | upon_error([] (auto error) {});

    auto s1 = just(1) | then([](int) {}) | upon_error([] (auto error) {
        try {
            std::rethrow_exception(error);
        } catch (std::exception const& ex) {
            return std::string{ex.what()};
        }
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
        == meta::list<signature<>, signature<std::string>>{});
}

TEST(upon_error, test)
{
    auto make = [] (int x) {
        return just(x)
            | then([] (int i) {
                if (i == 1) {
                    throw std::runtime_error{":("};
                }
                return i * 2;
            })
            | upon_error([] (auto) {
                return 2;
            })
            | then([] (int x) {
                return x + 1;
            });
    };

    {
        auto [r] = *this_thread::sync_wait(make(1));
        EXPECT_EQ(3, r);
    }

    {
        auto [r] = *this_thread::sync_wait(make(2));
        EXPECT_EQ(5, r);
    }
}
