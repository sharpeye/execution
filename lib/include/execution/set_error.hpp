#pragma once

#include "customization.hpp"

#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

inline constexpr struct set_error_fn
{
    // default implementation
    template <typename R, typename E>
        requires (!is_tag_invocable_v<set_error_fn, R&&, E&&>)
    void operator () (R&& receiver, E&& error) const
    {
        std::forward<R>(receiver).set_error(std::forward<E>(error));
    }

    template <typename R, typename E>
        requires is_tag_invocable_v<set_error_fn, R&&, E&&>
    void operator () (R&& receiver, E&& error) const
    {
        return execution::tag_invoke(
            *this,
            std::forward<R>(receiver),
            std::forward<E>(error));
    }

} set_error;

}   // namespace execution
