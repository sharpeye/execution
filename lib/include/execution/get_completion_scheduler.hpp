#pragma once

#include "customization.hpp"

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct get_completion_scheduler_fn
{
    template <typename S>
    auto operator () (S const& sender) const
    {
        return execution::tag_invoke(*this, sender);
    }

};

template <typename T>
constexpr auto get_completion_scheduler = get_completion_scheduler_fn<T> {};

}   // namespace execution
