#pragma once

#include <stop_token>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
auto get_stop_token(R& receiver) -> std::stop_token
{
    (void) receiver;
    return {};
}

}   // namespace execution
