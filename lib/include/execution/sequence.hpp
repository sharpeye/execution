#pragma once

#include "sender_traits.hpp"
#include "variant.hpp"

#include <functional>

namespace execution {
namespace sequence_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename T, int I>
struct receiver
{
    T* _operation;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        _operation->set_value(meta::number<I>, std::forward<Ts>(values)...);
    }

    template <typename E>
    void set_error(E&& error)
    {
        _operation->set_error(std::forward<E>(error));
    }

    void set_stopped()
    {
        _operation->SetStopped();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename ... Ts>
struct operation
{
    static constexpr auto indices = meta::iota<sizeof ... (Ts)>;
    static constexpr auto sender_types = meta::list<Ts...>{};
    static constexpr auto receiver_types = meta::transform(
        indices,
        [] <int I> (meta::number_t<I>) {
            return meta::atom<receiver<operation, I>>{};
        });
    static constexpr auto operation_types = meta::transform(
        indices,
        [] (auto index) {
            return traits::sender_operation(
                sender_types[index],
                receiver_types[index]
            );
        });

    using state_t = variant_t<decltype(
          operation_types
        | meta::atom<std::monostate>{}
    )>;

    R _receiver;
    std::tuple<Ts...> _senders;
    state_t _state;

    template <typename U>
    explicit operation(U&& receiver, std::tuple<Ts...>&& senders)
        : _receiver(std::forward<U>(receiver))
        , _senders(std::move(senders))
        , _state(std::monostate{})
    {}

    void start()
    {
        start_next<0>();
    }

    template <int I, typename ... Us>
    void set_value(meta::number_t<I>, Us&& ... values)
    {
        if constexpr (I + 1 < sizeof ... (Ts)) {
            static_assert(sizeof ... (Us) == 0);
            start_next<I + 1>();
        } else {
            execution::set_value(
                std::move(_receiver),
                std::forward<Us>(values)...
            );
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

    template <int I>
    void start_next()
    {
        auto& op = _state.template emplace<I>(execution::connect(
            std::get<I>(std::move(_senders)),
            receiver<operation, I>{this}
        ));

        op.start();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename ... Ts>
struct sender
{
    std::tuple<T, Ts...> _senders;

    template <typename U, typename ... Us>
    explicit sender(U&& s0, Us&& ... ss)
        : _senders{ std::forward<U>(s0), std::forward<Us>(ss)... }
    {}

    template <typename R>
    auto connect(R&& receiver)
    {
        return operation<R, T, Ts...>{
            std::forward<R>(receiver),
            std::move(_senders)
        };
    }
};

template <typename T, typename ... Ts>
sender(T&&, Ts&& ...) -> sender<std::decay_t<T>, std::decay_t<Ts>...>;

///////////////////////////////////////////////////////////////////////////////

struct sequence
{
    template <typename T, typename ... Ts>
    constexpr auto operator () (T&& s0, Ts&& ... ss) const
    {
        return sender{
            std::forward<T>(s0),
            std::forward<Ts>(ss)...
        };
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename ... Ts>
struct sender_traits
{
    static constexpr auto sender_types = meta::list<T, Ts...>{};
    static constexpr auto indices = meta::iota<1 + sizeof ... (Ts)>;

    template <typename R>
    struct with
    {
        using operation_t = operation<R, T, Ts...>;

        static constexpr auto receiver_type = meta::atom<R>{};
        static constexpr auto operation_type = meta::atom<operation<R, T, Ts...>>{};
        static constexpr auto receiver_types = meta::transform(
            indices,
            [] <int I> (meta::number_t<I>) {
                return meta::atom<receiver<operation_t, I>>{};
            });

        static constexpr auto value_types = traits::sender_values(
            sender_types[meta::number<-1>],
            receiver_types[meta::number<-1>]
        );

        static constexpr auto error_types = meta::transform_unique(
            indices, [] (auto index) {
                return traits::sender_errors(
                    sender_types[index],
                    receiver_types[index]
                );
            });
    };
};

}   // namespace sequence_impl

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename ... Ts>
struct sender_traits<sequence_impl::sender<T, Ts...>>
    : sequence_impl::sender_traits<T, Ts...>
{};

///////////////////////////////////////////////////////////////////////////////

constexpr auto sequence = sequence_impl::sequence{};

}   // namespace execution
