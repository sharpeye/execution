#pragma once

#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "stop_token.hpp"

#include <exception>
#include <functional>

namespace execution {

namespace upon_stopped_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename R>
struct upon_stopped_receiver
{
    F _func;
    R _receiver;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        execution::set_value(
            std::move(_receiver),
            std::forward<Ts>(values)...
        );
    }

    template <typename E>
    void set_error(E&& error)
    {
        execution::set_error(
            std::move(_receiver),
            std::forward<E>(error));
    }

    void set_stopped()
    {
        execution::set_value_with(std::move(_receiver), std::move(_func));
    }

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, upon_stopped_receiver<F, R> const& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return std::invoke(tag, self._receiver, std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct sender
{
    P _predecessor;
    F _func;

    template <typename U, typename V>
    sender(U&& p, V&& func)
        : _predecessor(std::forward<U>(p))
        , _func(std::forward<V>(func))
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return execution::connect(
            _predecessor,
            upon_stopped_receiver<F, R>{
                std::move(_func),
                std::forward<R>(receiver)
            });
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return execution::connect(
            std::move(_predecessor),
            upon_stopped_receiver<F, R>{
                std::move(_func),
                std::forward<R>(receiver)
            });
    }
};

////////////////////////////////////////////////////////////////////////////////

struct upon_stopped
{
    template <typename F>
    constexpr auto operator () (F&& func) const
    {
        return pipeable(*this, std::forward<F>(func));
    }

    template <typename P, typename F>
    constexpr auto operator () (P&& predecessor, F&& func) const
    {
        return sender<std::decay_t<P>, std::decay_t<F>>{
            std::forward<P>(predecessor),
            std::forward<F>(func)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct sender_traits
{
    static constexpr auto predecessor_type = meta::atom<P>{};
    static constexpr auto receiver_type = meta::atom<upon_stopped_receiver<F, R>>{};
    static constexpr auto operation_type = traits::sender_operation(
        predecessor_type, receiver_type);
    static constexpr auto value_types = meta::concat_unique(
            traits::sender_values(predecessor_type, receiver_type),
            traits::invoke_result_as_signature<F>()
        );
    static constexpr auto error_types = meta::concat_unique(
        traits::sender_errors(predecessor_type, receiver_type),
        meta::list<std::exception_ptr>{} // TODO: F noexcept
    );

    using operation_t = typename decltype(operation_type)::type;
    using errors_t = decltype(error_types);
    using values_t = decltype(value_types);
};

}   // namespace upon_stopped_impl

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct sender_traits<upon_stopped_impl::sender<P, F>, R>
    : upon_stopped_impl::sender_traits<P, F, R>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto upon_stopped = upon_stopped_impl::upon_stopped{};

}   // namespace execution
