#pragma once

#include <cstddef>
#include <functional>
#include <type_traits>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <
    typename S = std::nullptr_t,
    typename F = std::nullptr_t,
    typename C = std::nullptr_t
>
struct test_receiver
{
    S _set_value;
    F _set_error;
    C _set_stopped;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        if constexpr (std::is_invocable_v<S, Ts...>) {
            std::invoke(_set_value, std::forward<Ts>(values)...);
        }
    }

    template <typename E>
    void set_error(E&& error)
    {
        if constexpr (std::is_invocable_v<F, E>) {
            std::invoke(_set_error, std::forward<E>(error));
        }
    }

    void set_stopped()
    {
        if constexpr (std::is_invocable_v<C>) {
            std::invoke(_set_stopped);
        }
    }

    template <typename H>
    test_receiver<H, F, C> on_value(H&& func)
    {
        return {
            ._set_value = std::forward<H>(func),
            ._set_error = std::move(_set_error),
            ._set_stopped = std::move(_set_stopped)
        };
    }

    template <typename H>
    test_receiver<S, H, C> on_error(H&& func)
    {
        return {
            ._set_value = std::move(_set_value),
            ._set_error = std::forward<H>(func),
            ._set_stopped = std::move(_set_stopped)
        };
    }

    template <typename H>
    test_receiver<S, F, H> on_stopped(H&& func)
    {
        return {
            ._set_value = std::move(_set_value),
            ._set_error = std::move(_set_error),
            ._set_stopped =  std::forward<H>(func)
        };
    }
};

}   // namespace execution
