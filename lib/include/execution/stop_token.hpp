#pragma once

#include "customization.hpp"

#include <stop_token>
#include <type_traits>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

inline constexpr struct get_stop_token_fn
{
    // default implementation
    template <typename R>
        requires (!is_tag_invocable_v<get_stop_token_fn, R const&>)
    auto operator () (R const&) const noexcept
        -> std::stop_token
    {
        return {};
    }

    template <typename R>
        requires is_tag_invocable_v<get_stop_token_fn, R const&>
    auto operator () (R const& obj) const noexcept
        -> tag_invoke_result_t<get_stop_token_fn, R const&>
    {
        return execution::tag_invoke(*this, obj);
    }

} get_stop_token;

}   // namespace execution
