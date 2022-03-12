#pragma once

#include <exception>

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

struct TNullReceiver
{
    template <typename ... Ts>
    void SetValue(Ts&& ...)
    {}

    template <typename E>
    void SetError(E&&)
    {
        std::terminate();
    }

    void SetStopped()
    {
        std::terminate();
    }
};

}   // namespace NExecution
