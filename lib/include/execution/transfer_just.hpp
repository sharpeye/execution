#pragma once

#include "get_completion_scheduler.hpp"
#include "get_scheduler.hpp"
#include "sender_traits.hpp"

#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace execution {
namespace transfer_just_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename T>
struct operation;

template <typename R, typename S, typename T>
struct receiver
{
    operation<R, S, T>* _op;

    void set_value()
    {
        _op->set_value();
    }

    template <typename E>
    void set_error(E&& error)
    {
        _op->set_error(std::forward<E>(error));
    }

    void set_stopped()
    {
        _op->set_stopped();
    }

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, receiver<R, S, T> const& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return std::invoke(tag, self._op->_receiver, std::forward<Ts>(args)...);
    }
};

template <typename R, typename S, typename T>
struct operation
{
    using schedule_receiver_t = receiver<R, S, T>;
    using schedule_sender_t = decltype(std::declval<S>().schedule());
    using schedule_operation_t = typename sender_traits<
        schedule_sender_t,
        schedule_receiver_t>::operation_t;

    R _receiver;
    S _scheduler;
    T _values;
    std::optional<schedule_operation_t> _schedule_operation;

    void start() & noexcept
    {
        auto& op = _schedule_operation.emplace(
            execution::connect(
                _scheduler.schedule(),
                receiver<R, S, T>{this}
            ));
        execution::start(op);
    }

    void set_value() noexcept
    {
        execution::apply_value(std::move(_receiver), std::move(_values));
    }

    template <typename E>
    void set_error(E&& error)
    {
        execution::set_error(std::move(_receiver), std::forward<E>(error));
    }

    void set_stopped()
    {
        execution::set_stopped(std::move(_receiver));
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename ... Ts>
struct sender
{
    using tuple_t = std::tuple<Ts...>;

    S _scheduler;
    tuple_t _values;

    template <typename T, typename ... Us>
    explicit sender(T scheduler, Us&& ... values)
        : _scheduler {std::forward<T>(scheduler)}
        , _values {std::forward<Us>(values)...}
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<R, S, tuple_t>{
            std::forward<R>(receiver),
            _scheduler,
            _values
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<R, S, tuple_t>{
            std::forward<R>(receiver),
            std::move(_scheduler),
            std::move(_values)
        };
    }
    
    friend auto tag_invoke(
        tag_t<get_completion_scheduler<set_value_t>>,
        sender<S, Ts...> const& self)
    {
        return self._scheduler;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct transfer_just
{
    template <typename S, typename ... Ts>
    constexpr auto operator () (S&& scheduler, Ts&& ... values) const
    {
        return sender<std::decay_t<S>, std::decay_t<Ts>...> {
            std::forward<S>(scheduler),
            std::forward<Ts>(values)...
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename ... Ts>
struct sender_traits
{
    using operation_t = operation<R, S, std::tuple<Ts...>>;
    using values_t = meta::list<signature<Ts...>>;
    using errors_t =
        // TODO
        meta::list<std::exception_ptr>;
};

}   // namespace transfer_just_impl

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename ... Ts>
struct sender_traits<transfer_just_impl::sender<S, Ts...>, R>
    : transfer_just_impl::sender_traits<R, S, Ts...>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto transfer_just = transfer_just_impl::transfer_just{};

}   // namespace execution
