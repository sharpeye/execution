#pragma once

#include "customization.hpp"
#include "sender_traits.hpp"

#include <variant>

namespace execution {
namespace on_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename I>
struct operation;

template <typename R, typename S, typename I>
struct receiver
{
    operation<R, S, I>* op;

    void set_value()
    {
        op->set_value();
    }

    template <typename E>
    void set_error(E&& error)
    {
        op->set_error(std::forward<E>(error));
    }

    void set_stopped()
    {
        op->set_stopped();
    }

    // tag_invoke

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, const receiver<R, S, I>& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return tag(self.op->_receiver, std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename I>
struct operation
{
    using receiver_t = receiver<R, S, I>;
    using schedule_op_t = typename sender_traits<S, receiver_t>::operation_t;
    using input_op_t = typename sender_traits<I, R>::operation_t;

    using state_t = std::variant<
        S,
        schedule_op_t,
        input_op_t
    >;

    R _receiver;
    state_t _state;
    I _input;

    void start()
    {
        auto schedule = std::get<S>(std::move(_state));
        auto& op = _state.template emplace<schedule_op_t>(
            execution::connect(std::move(schedule), receiver_t {this}));
        execution::start(op);
    }

    void set_value()
    {
        auto& op = _state.template emplace<input_op_t>(
            execution::connect(std::move(_input), std::move(_receiver))
        );
        execution::start(op);
    }

    template <typename E>
    void set_error(E&& error)
    {
        execution::set_error(std::move(_receiver), std::move(error));
    }

    void set_stopped()
    {
        execution::set_stopped(std::move(_receiver));
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename I>
struct sender
{
    S _scheduler;
    I _input;

    template <typename R>
    auto connect(R&& receiver) &
    {
        using T = std::decay_t<decltype(_scheduler.schedule())>;

        return operation<std::decay_t<R>, T, I> {
            std::forward<R>(receiver),
            _scheduler.schedule(),
            _input
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        using T = std::decay_t<decltype(std::declval<S>().schedule())>;

        return operation<std::decay_t<R>, T, I> {
            std::forward<R>(receiver),
            std::move(_scheduler).schedule(),
            std::move(_input)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename I>
struct sender_traits
{
    using schedule_sender_t = std::decay_t<decltype(std::declval<S>().schedule())>;
    using operation_t = operation<R, schedule_sender_t, I>;
    using receiver_t = receiver<R, schedule_sender_t, I>;

    using values_t = typename execution::sender_traits<I, R>::values_t;
    using errors_t = decltype(concat_unique(
        traits::sender_errors(meta::atom<I> {}, meta::atom<R> {}),
        traits::sender_errors(meta::atom<schedule_sender_t> {}, meta::atom<receiver_t> {})
    ));
};

////////////////////////////////////////////////////////////////////////////////

struct on
{
    template <typename S, typename I>
    constexpr auto operator () (S&& scheduler, I&& input) const
    {
        return sender<std::decay_t<S>, std::decay_t<I>> {
            std::forward<S>(scheduler),
            std::forward<I>(input)
        };
    }
};

}   // namespace on_impl

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename I>
struct sender_traits<on_impl::sender<S, I>, R>
    : on_impl::sender_traits<R, S, I>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto on = on_impl::on {};

}   // namespace execution
