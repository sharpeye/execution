#pragma once

#include <exception>

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

struct TNullReceiver
{
    template <typename ... Ts>
    void OnSuccess(Ts&& ...)
    {}

    template <typename E>
    void OnFailure(E&&)
    {
        std::terminate();
    }

    void OnCancel()
    {
        std::terminate();
    }
};

}   // namespace NExecution
