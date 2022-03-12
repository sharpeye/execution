#pragma once

#include "sender_traits.h"

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct TForwardReceiver
{
    T* Op;

    template <typename ... Ts>
    void SetValue(Ts&& ... values)
    {
        Op->SetValue(std::forward<Ts>(values)...);
    }

    template <typename E>
    void SetError(E&& error)
    {
        Op->SetError(std::forward<E>(error));
    }

    void SetStopped()
    {
        Op->SetStopped();
    }
};

}   // namespace NExecution
