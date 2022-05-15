#pragma once

#include "customization.hpp"
#include "get_completion_scheduler.hpp"
#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "variant.hpp"

namespace execution {
namespace bulk_impl {

template <typename R, typename S, typename I, typename F>
struct operation;

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename I, typename F>
struct receiver
{
    operation<R, S, I, F>* _operation;

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

    // tag_invoke

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, const receiver<R, S, I, F>& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return tag(self._operation->_receiver, std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename I, typename F>
struct operation
{
    using receiver_t = receiver<R, S, I, F>;

    static constexpr auto predecessor_type = meta::atom<S>{};
    static constexpr auto predecessor_receiver_type = meta::atom<receiver_t>{};
    static constexpr auto predecessor_operation_type = traits::sender_operation(
        predecessor_type,
        predecessor_receiver_type);

    using state_t = variant_t<decltype(
          predecessor_type
        | predecessor_operation_type
    )>;

    R _receiver;
    I _shape;
    F _func;
    state_t _state;

    template <typename U, typename P, typename X>
    operation(U&& receiver, P&& predecessor, I shape, X&& func)
        : _receiver(std::forward<U>(receiver))
        , _shape(shape)
        , _func(std::forward<X>(func))
        , _state(std::in_place_index<0>, std::forward<P>(predecessor))
    {}

    void start() & noexcept
    {
        auto& op = _state.template emplace<1>(
            execution::connect(
                std::get<S>(std::move(_state)),
                receiver_t{this}
            ));

        execution::start(op);
    }

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        try {
            for (I i {}; i != _shape; ++i) {
                std::invoke(_func, i, values...);
            }

            execution::set_value(
                std::move(_receiver),
                std::forward<Ts>(values)...
            );
        }
        catch (...) {
            execution::set_error(std::move(_receiver), std::current_exception());
        }
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

template <typename S, typename I, typename F>
struct sender
{
    S _predecessor;
    I _shape;
    F _func;

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<R, S, I, F>{
            std::forward<R>(receiver),
            _predecessor,
            _shape,
            _func
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<R, S, I, F>{
            std::forward<R>(receiver),
            std::move(_predecessor),
            _shape,
            std::move(_func)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename I, typename F>
struct sender_traits
{
    static constexpr auto predecessor_type = meta::atom<S>{};
    static constexpr auto receiver_type = meta::atom<R>{};

    static constexpr auto value_types = traits::sender_values(
        predecessor_type,
        receiver_type);
    static constexpr auto error_types = meta::concat_unique(
        traits::sender_errors(predecessor_type, receiver_type),
        meta::list<std::exception_ptr>{}
    );

    using operation_t = operation<R, S, I, F>;
    using values_t = decltype(value_types);
    using errors_t = decltype(error_types);
};

}   // namespace bulk_impl

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename I, typename F>
struct sender_traits<bulk_impl::sender<S, I, F>, R>
    : bulk_impl::sender_traits<R, S, I, F>
{};

////////////////////////////////////////////////////////////////////////////////

inline constexpr struct bulk_fn
{
    template <typename I, typename F>
        requires std::is_integral_v<I>
    auto operator () (I shape, F&& func) const
    {
        return pipeable(*this, shape, std::forward<F>(func));
    }

    // default implementation
    template <typename S, typename I, typename F>
        requires (std::is_integral_v<I>
            && !is_tag_invocable_v<bulk_fn, S&&, I, F&&>
            && !is_tag_invocable_v<get_completion_scheduler_fn<set_value_t>, S const&>)
    auto operator () (S&& sender, I shape, F&& func) const
    {
        return bulk_impl::sender<std::decay_t<S>, std::decay_t<I>, std::decay_t<F>> {
            std::forward<S>(sender),
            shape,
            std::forward<F>(func)
        };
    }

    template <typename S, typename I, typename F>
        requires (std::is_integral_v<I>
            && !is_tag_invocable_v<get_completion_scheduler_fn<set_value_t>, S const&>
            && is_tag_invocable_v<bulk_fn, S&&, I, F&&>)
    auto operator () (S&& sender, I shape, F&& func) const
    {
        return execution::tag_invoke(
            *this,
            std::forward<S>(sender),
            shape,
            std::forward<F>(func));
    }

    template <typename S, typename I, typename F>
        requires (std::is_integral_v<I>
            && is_tag_invocable_v<get_completion_scheduler_fn<set_value_t>, S const&>
            && is_tag_invocable_v<
                bulk_fn,
                decltype(get_completion_scheduler<set_value_t>(std::declval<S>())),
                S&&,
                I,
                F&&>)
    auto operator () (S&& sender, I shape, F&& func) const
    {
        return execution::tag_invoke(
            *this,
            get_completion_scheduler<set_value_t>(sender),
            std::forward<S>(sender),
            shape,
            std::forward<F>(func));
    }

} bulk;

}   // namespace execution
