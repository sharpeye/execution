#pragma once

#include <functional>

namespace execution {

///////////////////////////////////////////////////////////////////////////////

template <typename F>
struct pipeable_func : F
{};

template <typename F>
pipeable_func(F) -> pipeable_func<F>;

template <typename F, typename ... Ts>
auto pipeable(F func, Ts&& ... args)
{
    return pipeable_func{[func, ...args = std::forward<Ts>(args)] (auto&& arg0)
        {
            return std::invoke(
                func,
                std::forward<decltype(arg0)>(arg0),
                std::move(args)...
            );
        }};
}

template <typename P, typename T>
auto operator | (P&& predecessor, pipeable_func<T>&& p)
{
    return std::invoke(std::move(p), std::forward<P>(predecessor));
}

}   // namespace execution
