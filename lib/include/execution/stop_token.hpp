#pragma once

#include <stop_token>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
    requires (sizeof(&R::get_stop_token) != 0)
auto get_stop_token(R& receiver) -> std::stop_token
{
    return receiver.get_stop_token();
}

template <typename R>
auto get_stop_token(R& receiver) -> std::stop_token
{
    (void) receiver;

    return {};
}

}   // namespace execution
