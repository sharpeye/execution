#pragma once

#include "customization.hpp"

#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

inline constexpr struct get_scheduler_fn
{
    template <typename R>
        requires is_tag_invocable_v<get_scheduler_fn, R&&>
    void operator () (R&& receiver) const
    {
        return execution::tag_invoke(*this, std::forward<R>(receiver));
    }

} get_scheduler;

using get_scheduler_t = get_scheduler_fn;

}   // namespace execution
