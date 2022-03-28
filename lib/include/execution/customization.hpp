#pragma once

#include <type_traits>
#include <utility>

namespace execution {

namespace tag_invoke_fn_ns {

////////////////////////////////////////////////////////////////////////////////

void tag_invoke() /*= delete*/ ;

struct tag_invoke_fn
{
    template <typename Tag, typename ... Ts>
    constexpr auto operator() (Tag tag, Ts&& ... args) const
        noexcept(noexcept(tag_invoke(tag, std::forward<Ts>(args)...)))
        -> decltype(tag_invoke(tag, std::forward<Ts>(args)...))
    {
        return tag_invoke(tag, std::forward<Ts>(args)...);
    }
};

template <typename Tag, typename ... Ts>
using tag_invoke_result_t = decltype(
    tag_invoke(std::declval<Tag>(), std::declval<Ts>()...));

}   // namespace tag_invoke_fn_ns

////////////////////////////////////////////////////////////////////////////////

inline namespace tag_invoke_cpo_ns {

inline constexpr tag_invoke_fn_ns::tag_invoke_fn tag_invoke{};

}   // namespace tag_invoke_cpo_ns

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename ... Ts>
consteval bool is_tag_invocable()
{
    return false;
}

template<typename T, typename ... Ts>
    requires requires (T tag, Ts&& ... args) { tag_invoke(tag, std::forward<Ts>(args)...); }
consteval bool is_tag_invocable()
{
    return true;
}

template <typename Tag, typename ... Ts>
inline constexpr bool is_tag_invocable_v = is_tag_invocable<Tag, Ts...>();

template <typename Tag, typename ... Ts>
inline constexpr bool is_nothrow_tag_invocable_v = noexcept(
    tag_invoke(std::declval<Tag>(), std::declval<Ts>()...));

////////////////////////////////////////////////////////////////////////////////

using tag_invoke_fn_ns::tag_invoke_result_t;

template <auto Tag>
using tag_t = std::remove_cvref_t<decltype(Tag)>;

}   // namespace execution
