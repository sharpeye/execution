#pragma once

#include "meta.hpp"
#include "set_error.hpp"
#include "set_stopped.hpp"
#include "set_value.hpp"

#include <exception>
#include <functional>
#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename R>
struct sender_traits
{
    using operation_t = typename S::template operation_t<R>;
    using values_t = typename S::values_t;
    using errors_t = typename S::errors_t;
};

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
constexpr auto invoke_result_as_signature()
{
    if constexpr (std::is_void_v<std::invoke_result_t<F, Ts...>>) {
        return meta::atom<signature<>>{};
    } else {
        return meta::atom<signature<std::invoke_result_t<F, Ts...>>>{};
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename ... Ts>
constexpr bool is_nothrow_invocable(meta::atom<F>, meta::atom<signature<Ts...>>)
{
    return std::is_nothrow_invocable_v<F, Ts...>;
}

}   // namespace traits

////////////////////////////////////////////////////////////////////////////////

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
