#pragma once

#include "customization.hpp"
#include "pipeable.hpp"
#include "sender_traits.hpp"

#include <optional>

namespace execution {
namespace repeat_effect_until_impl {

template <typename R, typename S, typename F>
struct operation;

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename F>
struct source_receiver
{
    operation<R, S, F>* _operation;

    void set_value()
    {
        _operation->set_value();
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
    friend auto tag_invoke(Tag tag, const source_receiver<R, S, F>& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return tag(self._operation->get_receiver(), std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename F>
struct operation
{
    using source_receiver_t = source_receiver<R, S, F>;

    static constexpr auto source_type = meta::atom<S>{};
    static constexpr auto source_receiver_type = meta::atom<source_receiver_t>{};
    static constexpr auto source_operation_type = traits::sender_operation(
        source_type, source_receiver_type);
    static constexpr auto source_value_types = traits::sender_values(
        source_type, source_receiver_type);

    static_assert(source_value_types == meta::list<signature<>>{});

    using state_t = std::optional<typename decltype(
        source_operation_type
    )::type>;

    R _receiver;
    S _source;
    F _condition;

    state_t _state;

    template <typename T, typename U, typename V>
    operation(T&& receiver, U&& src, V&& cond)
        : _receiver(std::forward<T>(receiver))
        , _source(std::forward<U>(src))
        , _condition(std::forward<V>(cond))
    {}

    void start() & noexcept
    {
        restart();
    }

    void set_value()
    {
        _state.reset();

        if (std::invoke(_condition)) {
            execution::set_value(std::move(_receiver));
        } else {
            restart();
        }
    }

    template <typename E>
    void set_error(E&& error)
    {
        _state.reset();
        execution::set_error(std::move(_receiver), std::forward<E>(error));
    }

    void set_stopped()
    {
        _state.reset();
        execution::set_stopped(std::move(_receiver));
    }

    R const& get_receiver() const
    {
        return _receiver;
    }

    void restart()
    {
        _state.emplace(execution::connect(_source, source_receiver_t{this}));
        _state->start();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename F>
struct sender
{
    S _source;
    F _condition;

    template <typename T, typename U>
    sender(T&& src, U&& cond)
        : _source(std::forward<T>(src))
        , _condition(std::forward<U>(cond))
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<R, S, F>{
            std::forward<R>(receiver),
            _source,
            _condition
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<R, S, F>{
            std::forward<R>(receiver),
            std::move(_source),
            std::move(_condition)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct repeat_effect_until
{
    template <typename F>
    constexpr auto operator () (F&& condition) const
    {
        return pipeable(*this, std::forward<F>(condition));
    }

    template <typename S, typename F>
    constexpr auto operator () (S&& source, F&& condition) const
    {
        return sender<std::decay_t<S>, std::decay_t<F>>(
            std::forward<S>(source),
            std::forward<F>(condition)
        );
    }
};

struct repeat_effect
{
    static bool never() noexcept
    {
        return false;
    }

    constexpr auto operator () () const
    {
        return repeat_effect_until{}(&repeat_effect::never);
    }

    template <typename S>
    constexpr auto operator () (S&& source) const
    {
        return repeat_effect_until{}(
            std::forward<S>(source),
            &repeat_effect::never
        );
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename F>
struct sender_traits
{
    using operation_t = operation<R, S, F>;
    using values_t = meta::list<signature<>>;
    using errors_t = meta::list<std::exception_ptr>;
};

}   // namespace repeat_effect_until_impl

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename F>
struct sender_traits<repeat_effect_until_impl::sender<S, F>, R>
    : repeat_effect_until_impl::sender_traits<R, S, F>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto repeat_effect_until = repeat_effect_until_impl::repeat_effect_until{};
constexpr auto repeat_effect = repeat_effect_until_impl::repeat_effect{};

}   // namespace execution
