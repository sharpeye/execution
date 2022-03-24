#pragma once

#include <utility>

namespace execution {
namespace schedule_impl {

////////////////////////////////////////////////////////////////////////////////

struct schedule
{
    template <typename S>
    constexpr auto operator () (S&& scheduler) const
    {
        return std::forward<S>(scheduler).schedule();
    }
};

struct schedule_after
{
    template <typename S, typename D>
    constexpr auto operator () (S&& scheduler, D duration) const
    {
        return std::forward<S>(scheduler).schedule_after(duration);
    }
};

struct schedule_at
{
    template <typename S, typename P>
    constexpr auto operator () (S&& scheduler, P time_point) const
    {
        return std::forward<S>(scheduler).schedule_at(time_point);
    }
};

}   // namespace schedule_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto schedule = schedule_impl::schedule{};
constexpr auto schedule_after = schedule_impl::schedule_after{};
constexpr auto schedule_at = schedule_impl::schedule_at{};

}   // namespace execution
