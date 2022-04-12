#pragma once

#include "receiver_of.hpp"
#include "sender_traits.hpp"

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace execution {
namespace sender_of_impl {

////////////////////////////////////////////////////////////////////////////////

class operation
{
    struct base
    {
        virtual ~base() = default;
        virtual void start() = 0;
    };

    template <typename T>
    struct impl
        : base
    {
        T _op;

        template <typename U>
        explicit impl(U&& arg)
            : _op {std::forward<U>(arg)}
        {}

        void start() override
        {
            execution::start(_op);
        }
    };

private:
    std::unique_ptr<base> _impl;

public:
    template <typename T, typename U>
    explicit operation(std::in_place_type_t<T>, U&& arg)
        : _impl {new impl<T> {
            std::forward<U>(arg)
        }}
    {}

    void start() &
    {
        _impl->start();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename E>
struct sender_concept
{
    virtual ~sender_concept() = default;

    virtual operation multi_connect(receiver_of<T, E> r) = 0;
    virtual operation single_connect(receiver_of<T, E> r) = 0;
};

template <typename S, typename T, typename E>
class sender_impl
    : public sender_concept<T, E>
{
    using receiver_t = receiver_of<T, E>;
    using operation_t = typename sender_traits<S, receiver_t>::operation_t;

private:
    S _sender;

public:
    template <typename U>
    explicit sender_impl(U&& s)
        : _sender{std::forward<U>(s)}
    {}

    operation multi_connect(receiver_t r) override
    {
        return operation {
            std::in_place_type<operation_t>,
            execution::connect(_sender, std::move(r))
        };
    }

    operation single_connect(receiver_t r) override
    {
        return operation {
            std::in_place_type<operation_t>,
            execution::connect(std::move(_sender), std::move(r))
        };
    }
};

}   // namespace sender_of_impl

template <typename T = void, typename E = std::exception_ptr>
struct sender_of
{
    std::unique_ptr<sender_of_impl::sender_concept<T, E>> _impl;

    template <typename S>
    explicit sender_of(S&& sender)
        : _impl{new sender_of_impl::sender_impl<std::decay_t<S>, T, E>{
            std::forward<S>(sender)
        }}
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return _impl->multi_connect(receiver_of<T, E>{std::forward<R>(receiver)});
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return _impl->single_connect(receiver_of<T, E>{std::forward<R>(receiver)});
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename T, typename E>
struct sender_traits<sender_of<T, E>, R>
{
    using operation_t = sender_of_impl::operation;
    using values_t = meta::list<
        std::conditional_t<
            std::is_void_v<T>,
            signature<>,
            signature<T>
        >>;
    using errors_t = meta::list<E>;
};

}   // namespace execution
