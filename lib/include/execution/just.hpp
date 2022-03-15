#pragma once

#include "sender_traits.hpp"

#include <tuple>

namespace execution {
namespace just_impl {

///////////////////////////////////////////////////////////////////////////////

template <typename R, typename T>
struct operation
{
    R _receiver;
    T _values;

    template <typename U>
    operation(U&& receiver, T&& values)
        : _receiver(std::forward<U>(receiver))
        , _values(std::move(values))
    {}

    void start()
    {
        std::apply(
            [this] (auto&& ... values) {
                execution::set_value(
                    std::move(_receiver),
                    std::move(values)...
                );
            },
            std::move(_values)
        );
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct sender
{
    using tuple_t = std::tuple<Ts...>;

    tuple_t _values;

    template <typename ... Us>
    explicit sender(Us&& ... values)
        : _values(std::forward<Us>(values)...)
    {}

    template <typename R>
    auto connect(R&& receiver)
    {
        return operation<R, tuple_t>{
            std::forward<R>(receiver),
            std::move(_values)
        };
    }
};

///////////////////////////////////////////////////////////////////////////////

struct just
{
    template <typename ... Ts>
    constexpr auto operator () (Ts&& ... values) const
    {
        return just_impl::sender<std::decay_t<Ts>...>(
            std::forward<Ts>(values)...
        );
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct sender_traits
{
    template <typename R>
    struct with
    {
        using operation_t = operation<R, std::tuple<Ts...>>;

        static constexpr auto operation_type = meta::atom<operation_t>{};
        static constexpr auto value_types = meta::list<signature<Ts...>>{};
        static constexpr auto error_types = meta::nothing;
    };
};

}   // namespace just_impl

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct sender_traits<just_impl::sender<Ts...>>
    : just_impl::sender_traits<Ts...>
{};

///////////////////////////////////////////////////////////////////////////////

constexpr auto just = just_impl::just{};

}   // namespace execution
