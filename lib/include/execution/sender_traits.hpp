#pragma once

#include "meta.hpp"

#include <exception>
#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename S>
struct sender_traits;

template <typename ... Ts>
struct signature
{};

namespace traits {

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename R>
consteval auto sender_operation(meta::atom<S> = {}, meta::atom<R> = {})
{
    return sender_traits<S>::template with<R>::operation_type;
}

template <typename S, typename R>
consteval auto sender_errors(meta::atom<S> = {}, meta::atom<R> = {})
{
    return sender_traits<S>::template with<R>::error_types;
}

template <typename S, typename R>
consteval auto sender_values(meta::atom<S> = {}, meta::atom<R> = {})
{
    return sender_traits<S>::template with<R>::value_types;
}

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename ... Ts>
consteval auto invoke_result(meta::atom<F>, meta::atom<signature<Ts...>>)
{
    return meta::atom<std::invoke_result_t<F, Ts...>>{};
}

template <typename F, typename ... Ts>
consteval auto invoke_result(meta::atom<F>, meta::atom<Ts>...)
{
    return meta::atom<std::invoke_result_t<F, Ts...>>{};
}

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename ... Ts>
consteval bool is_nothrow_invocable(meta::atom<F>, meta::atom<signature<Ts...>>)
{
    return std::is_nothrow_invocable_v<F, Ts...>;
}

template <typename F, typename ... Ts>
consteval bool is_nothrow_invocable(meta::atom<F>, meta::atom<Ts...>)
{
    return std::is_nothrow_invocable_v<F, Ts...>;
}

}   // namespace traits

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename ... Ts>
constexpr void set_value(R&& receiver, Ts&& ... values)
{
    std::forward<R>(receiver).set_value(std::forward<Ts>(values)...);
}

template <typename R, typename F, typename ... Ts>
constexpr void set_value_with(R&& receiver, F&& func, Ts&& ... values)
{
    if constexpr (std::is_void_v<std::invoke_result_t<F, Ts...>>) {
        std::invoke(std::forward<F>(func), std::forward<Ts>(values)...);
        execution::set_value(std::forward<R>(receiver));
    } else {
        execution::set_value(
            std::forward<R>(receiver),
            std::invoke(std::forward<F>(func), std::forward<Ts>(values)...)
        );
    }
}

template <typename R, typename E>
constexpr void set_error(R&& receiver, E&& error)
{
    std::forward<R>(receiver).set_error(std::forward<E>(error));
}

template <typename R>
constexpr void set_stopped(R&& receiver)
{
    std::forward<R>(receiver).set_stopped();
}

template <typename S, typename R>
constexpr auto connect(S&& sender, R&& receiver)
{
    return std::forward<S>(sender).connect(std::forward<R>(receiver));
}

}   // namespace execution
