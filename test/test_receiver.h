#pragma once

#include <functional>
#include <type_traits>

namespace NExecution {

////////////////////////////////////////////////////////////////////////////////

template <typename S = int, typename F = int, typename C = int>
struct TTestReceiver
{
    S SetValueCb;
    F SetErrorCb;
    C SetStoppedCb;

    template <typename ... Ts>
    void SetValue(Ts&& ... values)
    {
        if constexpr (std::is_invocable_v<S, Ts...>) {
            std::invoke(SetValueCb, std::forward<Ts>(values)...);
        }
    }

    template <typename E>
    void SetError(E&& error)
    {
        if constexpr (std::is_invocable_v<F, E>) {
            std::invoke(SetErrorCb, std::forward<E>(error));
        }
    }

    void SetStopped()
    {
        if constexpr (std::is_invocable_v<C>) {
            std::invoke(SetStoppedCb);
        }
    }

    template <typename H>
    TTestReceiver<H, F, C> OnValue(H&& func)
    {
        return {
            .SetValueCb = std::forward<H>(func),
            .SetErrorCb = std::move(SetErrorCb),
            .SetStoppedCb = std::move(SetStoppedCb)
        };
    }

    template <typename H>
    TTestReceiver<S, H, C> OnError(H&& func)
    {
        return {
            .SetValueCb = std::move(SetValueCb),
            .SetErrorCb = std::forward<H>(func),
            .SetStoppedCb = std::move(SetStoppedCb)
        };
    }

    template <typename H>
    TTestReceiver<S, F, H> OnStopped(H&& func)
    {
        return {
            .SetValueCb = std::move(SetValueCb),
            .SetErrorCb = std::move(SetErrorCb),
            .SetStoppedCb =  std::forward<H>(func)
        };
    }
};

}   // namespace NExecution
