#pragma once

#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "stop_token.hpp"

#include <exception>
#include <functional>

namespace execution {

namespace then_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename R>
struct then_receiver
{
    F _func;
    R _receiver;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        execution::set_value_with(
            std::move(_receiver),
            std::move(_func),
            std::forward<Ts>(values)...
        );
    }

    template <typename E>
    void set_error(E&& error)
    {
        execution::set_error(std::move(_receiver), std::forward<E>(error));
    }

    void set_stopped()
    {
        execution::set_stopped(std::move(_receiver));
    }

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, then_receiver<F, R> const& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return std::invoke(tag, self._receiver, std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct sender
{
    P _predecessor;
    F _func;

    template <typename U, typename V>
    sender(U&& p, V&& func)
        : _predecessor(std::forward<U>(p))
        , _func(std::forward<V>(func))
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return execution::connect(
            _predecessor,
            then_receiver<F, R>{
                std::move(_func),
                std::forward<R>(receiver)
            });
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return execution::connect(
            std::move(_predecessor),
            then_receiver<F, R>{
                std::move(_func),
                std::forward<R>(receiver)
            });
    }
};

////////////////////////////////////////////////////////////////////////////////

struct then
{
    template <typename F>
    constexpr auto operator () (F&& func) const
    {
        return pipeable(*this, std::forward<F>(func));
    }

    template <typename P, typename F>
    constexpr auto operator () (P&& predecessor, F&& func) const
    {
        return sender<std::decay_t<P>, std::decay_t<F>>{
            std::forward<P>(predecessor),
            std::forward<F>(func)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct sender_traits
{
    static constexpr auto predecessor_type = meta::atom<P>{};
    static constexpr auto func_type = meta::atom<F>{};

    static constexpr auto receiver_type = meta::atom<then_receiver<F, R>>{};

    static constexpr auto operation_type = traits::sender_operation(
        predecessor_type,
        receiver_type);

    static constexpr auto predecessor_value_types = traits::sender_values(
        predecessor_type,
        receiver_type);

    static constexpr auto get_invoke_result =
        [] <typename ... Ts> (meta::atom<signature<Ts...>>) {
            return meta::atom<signature<std::invoke_result_t<F, Ts...>>>{};
        };

    static constexpr auto value_types = 
        meta::replace(
            meta::transform_unique(
                traits::sender_values(predecessor_type, receiver_type),
                get_invoke_result
            ),
            meta::atom<signature<void>>{},
            meta::atom<signature<>>{});

    static constexpr bool is_nothrow = meta::is_all(
        traits::sender_values(predecessor_type, receiver_type),
        [] (auto values) {
            return traits::is_nothrow_invocable(func_type, values);
        });

    static constexpr auto error_types = meta::concat_unique(
        traits::sender_errors(predecessor_type, receiver_type),
        [] {
            if constexpr (is_nothrow) {
                return meta::list<>{};
            } else {
                return meta::list<std::exception_ptr>{};
            }
        } ());

    using operation_t = meta::type_t<operation_type>;
    using errors_t = decltype(error_types);
    using values_t = decltype(value_types);
};

}   // namespace then_impl

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct sender_traits<then_impl::sender<P, F>, R>
    : then_impl::sender_traits<P, F, R>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto then = then_impl::then{};

}   // namespace execution
