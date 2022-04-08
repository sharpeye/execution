#pragma once

#include "sender_traits.hpp"

#include <exception>
#include <future>
#include <memory>
#include <optional>

namespace execution {

namespace submit_impl {

////////////////////////////////////////////////////////////////////////////////

struct receiver
{
    std::shared_ptr<void> _state;

    void set_value()
    {
        _state.reset();
    }

    template <typename E>
    [[ noreturn ]] void set_error(E&&)
    {
        std::terminate();
    }

    [[ noreturn ]] void set_stopped()
    {
        std::terminate();
    }
};

template <typename T>
struct state
{
    std::optional<T> _operation;
};

struct submit
{
    template <typename S>
    void operator () (S&& sender) const
    {
        constexpr auto sender_type = meta::atom<std::decay_t<S>>{};
        constexpr auto receiver_type = meta::atom<receiver>{};
        constexpr auto value_types = traits::sender_values(
            sender_type, receiver_type);
        constexpr auto operation_type = traits::sender_operation(
            sender_type, receiver_type);

        static_assert(value_types == meta::list<signature<>>{});

        using T = typename decltype(operation_type)::type;

        auto s = std::make_shared<state<T>>();
        auto& op = s->_operation.emplace(
            execution::connect(std::forward<S>(sender), receiver{s}));

        op.start();
    }
};

}   // namespace submit_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto submit = submit_impl::submit{};

}   // namespace execution
