#pragma once

#include "meta.hpp"

#include <exception>
#include <functional>
#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename R>
struct sender_traits;

template <typename ... Ts>
struct signature
{};

namespace traits {

////////////////////////////////////////////////////////////////////////////////

constexpr auto sender_operation = [] <typename S, typename R> (
    meta::atom<S>,
    meta::atom<R>)
{
    return meta::atom<typename sender_traits<S, R>::operation_t>{};
};

constexpr auto sender_errors = [] <typename S, typename R> (
    meta::atom<S>,
    meta::atom<R>)
{
    return typename sender_traits<S, R>::errors_t {};
};

constexpr auto sender_values = [] <typename S, typename R> (
    meta::atom<S>,
    meta::atom<R>)
{
    return typename sender_traits<S, R>::values_t {};
};

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename ... Ts>
constexpr auto invoke_result(meta::atom<F>, meta::atom<signature<Ts...>>)
{
    return meta::atom<std::invoke_result_t<F, Ts...>>{};
}

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename ... Ts>
constexpr bool is_nothrow_invocable(meta::atom<F>, meta::atom<signature<Ts...>>)
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

template <typename T>
constexpr auto start(T& operation)
{
    operation.start();
}

}   // namespace execution
