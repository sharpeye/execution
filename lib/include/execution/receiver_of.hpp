#pragma once

#include "sender_traits.hpp"

#include <exception>
#include <memory>
#include <type_traits>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename E>
class receiver_of
{
    struct base
    {
        virtual ~base() = default;
        virtual void set_value(T value) = 0;
        virtual void set_error(E error) = 0;
        virtual void set_stopped() = 0;
    };

    template <typename R>
    struct impl
        : base
    {
        R _receiver;

        template <typename U>
        explicit impl(U&& r)
            : _receiver{std::forward<U>(r)}
        {}

        void set_value(T value) override
        {
            execution::set_value(std::move(_receiver), value);
        }

        void set_error(E error) override
        {
            execution::set_error(std::move(_receiver), error);
        }

        void set_stopped() override
        {
            execution::set_stopped(std::move(_receiver));
        }
    };

private:
    std::unique_ptr<base> _impl;

public:
    template <typename R>
    explicit receiver_of(R&& receiver)
        : _impl{new impl<std::decay_t<R>>{
            std::forward<R>(receiver)
        }}
    {}

    receiver_of(receiver_of&&) = default;
    receiver_of(receiver_of const&) = delete;

    receiver_of& operator = (receiver_of&&) = default;
    receiver_of& operator = (receiver_of const&) = delete;

    template <typename U>
    void set_value(U&& value)
    {
        _impl->set_value(std::forward<U>(value));
    }

    void set_error(E error)
    {
        _impl->set_error(error);
    }

    void set_stopped()
    {
        _impl->set_stopped();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename E>
class receiver_of<void, E>
{
    struct base
    {
        virtual ~base() = default;
        virtual void set_value() = 0;
        virtual void set_error(E error) = 0;
        virtual void set_stopped() = 0;
    };

    template <typename R>
    struct impl
        : base
    {
        R _receiver;

        template <typename U>
        explicit impl(U&& r)
            : _receiver{std::forward<U>(r)}
        {}

        void set_value() override
        {
            execution::set_value(std::move(_receiver));
        }

        void set_error(E error) override
        {
            execution::set_error(std::move(_receiver), error);
        }

        void set_stopped() override
        {
            execution::set_stopped(std::move(_receiver));
        }
    };

private:
    std::unique_ptr<base> _impl;

public:
    template <typename R>
    explicit receiver_of(R&& receiver)
        : _impl{new impl<std::decay_t<R>>{
            std::forward<R>(receiver)
        }}
    {}

    receiver_of(receiver_of&&) = default;
    receiver_of(receiver_of const&) = delete;

    receiver_of& operator = (receiver_of&&) = default;
    receiver_of& operator = (receiver_of const&) = delete;

    void set_value()
    {
        _impl->set_value();
    }

    void set_error(E error)
    {
        _impl->set_error(error);
    }

    void set_stopped()
    {
        _impl->set_stopped();
    }
};

}   // namespace execution
