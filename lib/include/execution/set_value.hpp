#pragma once

#include <exception>
#include <functional>
#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename ... Ts>
constexpr void set_value(R&& receiver, Ts&& ... values)
{
    std::forward<R>(receiver).set_value(std::forward<Ts>(values)...);
}

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
        set_error(std::forward<R>(receiver), std::current_exception());
    }
}

}   // namespace execution
