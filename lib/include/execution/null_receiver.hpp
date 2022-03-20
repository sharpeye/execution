#pragma once

#include <exception>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

struct null_receiver
{
    template <typename ... Ts>
    void set_value(Ts&& ...)
    {}

    template <typename E>
    [[ noreturn ]] void set_error(E&&)
    {
        std::terminate();
    }

    [[ noreturn ]] void set_stopped()
    {
        std::terminate();
    }
};

}   // namespace execution
