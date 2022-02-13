#pragma once

#include <exception>

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

struct TNullReceiver
{
    template <typename ... Ts>
    void OnSuccess(Ts&& ...)
    {}

    template <typename ... Ts>
    void OnFailure(Ts&& ...)
    {
        std::terminate();
    }

    void OnCancel()
    {
        std::terminate();
    }
};

}   // namespace NExecution
