#pragma once

#include <type_traits>
#include <utility>

namespace execution {

namespace tag_invoke_fn_ns {

////////////////////////////////////////////////////////////////////////////////

// poison pill
void tag_invoke() = delete;

struct tag_invoke_fn
{
    template<typename T, typename... Ts>
    constexpr auto operator () (T tag, Ts&&... args) const
        -> decltype(tag_invoke(tag, std::forward<Ts>(args)...))
    {
        return tag_invoke(tag, std::forward<Ts>(args)...);
    }
};

}   // namespace tag_invoke_fn_ns

////////////////////////////////////////////////////////////////////////////////

inline namespace tag_invoke_ns {

inline constexpr auto tag_invoke = tag_invoke_fn_ns::tag_invoke_fn{};

}   // namespace tag_invoke_ns

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename ... Ts>
using tag_invoke_result = std::invoke_result<decltype(execution::tag_invoke), T, Ts...>;

template<typename T, typename ... Ts>
using tag_invoke_result_t = std::invoke_result_t<decltype(execution::tag_invoke), T, Ts...>;

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename ... Ts>
consteval bool is_tag_invocable()
{
    return false;
}

template<typename T, typename ... Ts>
    requires requires (T tag, Ts&&...args) { tag_invoke(tag, std::forward<Ts>(args)...); }
consteval bool is_tag_invocable()
{
    return true;
}

template<typename T, typename ... Ts>
constexpr auto is_tag_invocable_v = is_tag_invocable<T, Ts...>();

////////////////////////////////////////////////////////////////////////////////

template<auto& tag>
using tag_t = std::decay_t<decltype(tag)>;

}   // namespace execution
