#pragma once

#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename E>
constexpr void set_error(R&& receiver, E&& error)
{
    std::forward<R>(receiver).set_error(std::forward<E>(error));
}

}   // namespace execution
