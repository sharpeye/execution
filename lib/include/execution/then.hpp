#pragma once

#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "stop_token.hpp"

#include <exception>
#include <functional>

namespace execution {

namespace then_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename R>
struct then_receiver
{
    F _func;
    R _receiver;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        execution::set_value_with(
            std::move(_receiver),
            std::move(_func),
            std::forward<Ts>(values)...
        );
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

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, then_receiver<F, R> const& self, Ts&& ... args)
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
            then_receiver<F, R>{
                std::move(_func),
                std::forward<R>(receiver)
            });
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return execution::connect(
            std::move(_predecessor),
            then_receiver<F, R>{
                std::move(_func),
                std::forward<R>(receiver)
            });
    }
};

////////////////////////////////////////////////////////////////////////////////

struct then
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

template <typename P, typename F>
struct sender_traits
{
    static constexpr auto predecessor_type = meta::atom<P>{};
    static constexpr auto func_type = meta::atom<F>{};

    template <typename R>
    struct with
    {
        static constexpr auto receiver_type = meta::atom<then_receiver<F, R>>{};

        static constexpr auto operation_type = traits::sender_operation(
            predecessor_type,
            receiver_type);

        static constexpr auto value_types = meta::remove(
            meta::transform_unique(
                traits::sender_values(predecessor_type, receiver_type),
                [] (auto sig) {
                    return to_signature(traits::invoke_result(func_type, sig));
                }
            ),
            meta::none
        );

        static constexpr auto error_types = meta::concat_unique(
            traits::sender_errors(predecessor_type, receiver_type),
            [] {
                constexpr bool nothrow = meta::is_all(
                    traits::sender_values(predecessor_type, receiver_type),
                    [] (auto sig) {
                        return traits::is_nothrow_invocable(func_type, sig);
                    }
                );

                if constexpr (nothrow) {
                    return meta::list<>{};
                } else {
                    return meta::list<std::exception_ptr>{};
                }
            } ());
    };
};

}   // namespace then_impl

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct sender_traits<then_impl::sender<P, F>>
    : then_impl::sender_traits<P, F>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto then = then_impl::then{};

}   // namespace execution
