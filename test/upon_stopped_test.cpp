#include <execution/upon_stopped.hpp>

#include <execution/just.hpp>
#include <execution/null_receiver.hpp>
#include <execution/schedule.hpp>
#include <execution/simple_thread_pool.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>

#include <gtest/gtest.h>

#include <string>

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

TEST(upon_stopped, traits)
{
    auto s0 = just() | upon_stopped([] {});

    auto s1 = just(1) | then([](int) {}) | upon_stopped([] {
        return std::string{};
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

TEST(upon_stopped, test)
{
    {
        auto [r] = *this_thread::sync_wait(just(3)
            | upon_stopped([] {
                return 42;
            }));

        EXPECT_EQ(3, r);
    }

    {
        simple_thread_pool pool{1};
        auto sched = pool.get_scheduler();

        std::stop_source ss;
        ss.request_stop();

        auto [r] = *this_thread::sync_wait(
            schedule(sched)
                | then([] { return 2; })
                | upon_stopped([] { return 41; })
                | then([] (int x) { return x + 1; }),
            ss);

        EXPECT_EQ(42, r);
    }
}
