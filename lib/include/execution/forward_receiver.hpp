#pragma once

#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct forward_receiver
{
    T* _operation;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        _operation->set_value(std::forward<Ts>(values)...);
    }

    template <typename E>
    void set_error(E&& error)
    {
        _operation->set_error(std::forward<E>(error));
    }

    void set_stopped()
    {
        _operation->set_stopped();
    }
};

}   // namespace execution
