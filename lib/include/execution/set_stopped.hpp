#pragma once

#include "customization.hpp"

#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

inline constexpr struct set_stopped_fn
{
    // default implementation
    template <typename R>
        requires (!is_tag_invocable_v<set_stopped_fn, R&&>)
    void operator () (R&& receiver) const
    {
        std::forward<R>(receiver).set_stopped();
    }

    template <typename R>
        requires is_tag_invocable_v<set_stopped_fn, R&&>
    void operator () (R&& receiver) const
    {
        return execution::tag_invoke(*this, std::forward<R>(receiver));
    }

} set_stopped;

}   // namespace execution
