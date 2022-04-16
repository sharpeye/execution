#pragma once

#include "sender_traits.hpp"

#include <utility>

namespace execution {
namespace just_stopped_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation
{
    R _receiver;

    template <typename U>
    operation(U&& receiver)
        : _receiver(std::forward<U>(receiver))
    {}

    void start() & noexcept
    {
        execution::set_stopped(std::move(_receiver));
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    template <typename R>
    using operation_t = operation<R>;
    using values_t = meta::list<signature<>>;
    using errors_t = meta::list<>;

    template <typename R>
    auto connect(R&& receiver)
    {
        return operation<R>{
            std::forward<R>(receiver),
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct just_stopped
{
    constexpr sender operator () () const
    {
        return {};
    }
};

}   // namespace just_stopped_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto just_stopped = just_stopped_impl::just_stopped{};

}   // namespace execution
