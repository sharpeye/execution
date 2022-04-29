#pragma once

#include "sender_traits.hpp"

#include <exception>
#include <future>
#include <memory>
#include <optional>

namespace execution {

namespace start_detached_impl {

////////////////////////////////////////////////////////////////////////////////

struct receiver
{
    std::shared_ptr<void> _storage;

    void set_value()
    {
        _storage.reset();
    }

    void set_stopped()
    {
        _storage.reset();
    }

    template <typename E>
    [[ noreturn ]] void set_error(E&&)
    {
        std::terminate();
    }
};

struct start_detached
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

        using operation_t = typename decltype(operation_type)::type;

        auto s = std::make_shared<std::optional<operation_t>>();

        auto& op = s->emplace(
            execution::connect(std::forward<S>(sender), receiver{s}));

        execution::start(op);
    }
};

}   // namespace start_detached_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto start_detached = start_detached_impl::start_detached{};

}   // namespace execution
