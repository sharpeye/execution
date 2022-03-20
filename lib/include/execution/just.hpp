#pragma once

#include "sender_traits.hpp"

#include <tuple>
#include <utility>

namespace execution {
namespace just_impl {

///////////////////////////////////////////////////////////////////////////////

template <typename R, typename T>
struct operation
{
    R _receiver;
    T _values;

    template <typename U, typename V>
    operation(U&& receiver, V&& values)
        : _receiver(std::forward<U>(receiver))
        , _values(std::forward<V>(values))
    {}

    void start() & noexcept
    {
        try {
            std::apply(
                [this] (auto&& ... values) {
                    execution::set_value(
                        std::move(_receiver),
                        std::move(values)...
                    );
                },
                std::move(_values)
            );
        } catch(...) {
            execution::set_error(std::move(_receiver), std::current_exception());
        }
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct sender
{
    using tuple_t = std::tuple<Ts...>;

    tuple_t _values;

    template <typename ... Us>
    explicit sender(std::in_place_t, Us&& ... values)
        : _values(std::forward<Us>(values)...)
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<R, tuple_t>{
            std::forward<R>(receiver),
            _values
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
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
            std::in_place,
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
        static constexpr auto error_types =
            // TODO
            meta::list<std::exception_ptr>{}
            ;
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
