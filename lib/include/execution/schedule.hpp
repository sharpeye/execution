#pragma once

#include "sender_traits.hpp"

#include <tuple>
#include <utility>

namespace execution {
namespace schedule_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename R>
struct operation
{
    S _scheduler;
    R _receiver;

    template <typename T, typename U>
    operation(T&& scheduler, U&& receiver)
        : _scheduler(std::forward<T>(scheduler))
        , _receiver(std::forward<U>(receiver))
    {}

    void start() & noexcept
    {
        // TODO
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S>
struct sender
{
    S _scheduler;

    template <typename T>
    explicit sender(T&& scheduler)
        : _scheduler(std::forward<T>(scheduler))
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<S, R>{
            _scheduler,
            std::forward<R>(receiver)
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<S, R>{
            std::forward<S>(_scheduler),
            std::forward<R>(receiver)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct schedule
{
    template <typename S>
    constexpr auto operator () (S&& scheduler) const
    {
        return schedule_impl::sender<std::decay_t<S>>(
            std::forward<S>(scheduler)
        );
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S>
struct sender_traits
{
    template <typename R>
    struct with
    {
        using operation_t = operation<S, R>;

        static constexpr auto operation_type = meta::atom<operation_t>{};
        static constexpr auto value_types = meta::list<signature<>>{};
        static constexpr auto error_types = meta::list<std::exception_ptr>{};
    };
};

}   // namespace schedule_impl

////////////////////////////////////////////////////////////////////////////////

template <typename S>
struct sender_traits<schedule_impl::sender<S>>
    : schedule_impl::sender_traits<S>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto schedule = schedule_impl::schedule{};

}   // namespace execution
