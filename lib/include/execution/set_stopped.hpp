#pragma once

#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
constexpr void set_stopped(R&& receiver)
{
    std::forward<R>(receiver).set_stopped();
}

}   // namespace execution
