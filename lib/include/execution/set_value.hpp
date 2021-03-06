#pragma once

#include "customization.hpp"
#include "set_error.hpp"

#include <exception>
#include <functional>
#include <tuple>
#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

inline constexpr struct set_value_fn
{
    // default implementation
    template <typename R, typename ... Ts>
        requires (!is_tag_invocable_v<set_value_fn, R&&, Ts&&...>)
    void operator () (R&& receiver, Ts&&... values) const
    {
        std::forward<R>(receiver).set_value(std::forward<Ts>(values)...);
    }

    template <typename R, typename ... Ts>
        requires is_tag_invocable_v<set_value_fn, R&&, Ts&&...>
    void operator () (R&& receiver, Ts&& ... values) const
    {
        return execution::tag_invoke(
            *this,
            std::forward<R>(receiver),
            std::forward<Ts>(values)...);
    }

} set_value;

using set_value_t = set_value_fn;

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename F, typename ... Ts>
constexpr auto set_value_with(R&& receiver, F&& func, Ts&& ... values) noexcept
{
    try {
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
    catch (...) {
        execution::set_error(std::forward<R>(receiver), std::current_exception());
    }
}

template <typename R, typename T>
constexpr auto apply_value(R&& receiver, T&& tuple) noexcept
{
    try {
        std::apply(
            [&receiver] (auto&& ... values) {
                execution::set_value(
                    std::forward<R>(receiver),
                    std::move(values)...
                );
            },
            std::forward<T>(tuple)
        );
    } catch(...) {
        execution::set_error(std::forward<R>(receiver), std::current_exception());
    }
}

}   // namespace execution
