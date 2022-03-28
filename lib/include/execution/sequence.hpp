#pragma once

#include "sender_traits.hpp"
#include "variant.hpp"

#include <functional>
#include <utility>

namespace execution {
namespace sequence_impl {

template <typename R, typename ... Ts>
struct operation;

////////////////////////////////////////////////////////////////////////////////

template <auto index, typename R, typename ... Ts>
struct receiver
{
    using self_t = receiver<index, R, Ts...>;

    operation<R, Ts...>* _operation;

    template <typename ... Us>
    void set_value(Us&& ... values)
    {
        _operation->set_value(index, std::forward<Us>(values)...);
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

    template <typename Tag, typename ... Us>
    friend auto tag_invoke(Tag tag, const self_t& self, Us&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Us...>)
        -> tag_invoke_result_t<Tag, R, Us...>
    {
        return tag(self._operation->get_receiver(), std::forward<Us>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename ... Ts>
struct operation
{
    template <auto index>
    using receiver_t = receiver<index, R, Ts...>;

    static constexpr auto indices = meta::iota<sizeof ... (Ts)>;
    static constexpr auto sender_types = meta::list<Ts...>{};
    static constexpr auto receiver_types = meta::transform(
        indices,
        [] (auto index) {
            return meta::atom<receiver_t<index>>{};
        });
    static constexpr auto operation_types = meta::zip_transform(
        sender_types, receiver_types, traits::sender_operation);

    using state_t = variant_t<
          operation_types
        | meta::atom<std::monostate>{}
    >;

    R _receiver;
    std::tuple<Ts...> _senders;
    state_t _state;

    template <typename U, typename S>
    explicit operation(U&& receiver, S&& senders)
        : _receiver(std::forward<U>(receiver))
        , _senders(std::forward<S>(senders))
        , _state(std::in_place_type<std::monostate>)
    {}

    void start() &
    {
        start_next<0>();
    }

    template <int i, typename ... Us>
    void set_value(meta::index_t<i>, Us&& ... values)
    {
        if constexpr (i + 1 < sizeof ... (Ts)) {
            static_assert(sizeof ... (Us) == 0);
            start_next<i + 1>();
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

    template <int i>
    void start_next()
    {
        auto& op = _state.template emplace<i>(execution::connect(
            std::get<i>(std::move(_senders)),
            receiver_t<meta::index_t<i>{}>{this}
        ));

        op.start();
    }

    auto const& get_receiver() const noexcept
    {
        return _receiver;
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct sender
{
    std::tuple<Ts...> _senders;

    template <typename ... Us>
    explicit sender(std::in_place_t, Us&& ... ss)
        : _senders{ std::forward<Us>(ss)... }
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<R, Ts...>{
            std::forward<R>(receiver),
            _senders
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<R, Ts...>{
            std::forward<R>(receiver),
            std::move(_senders)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sequence
{
    template <typename T, typename ... Ts>
    constexpr auto operator () (T&& s0, Ts&& ... ss) const
    {
        return sender<std::decay_t<T>, std::decay_t<Ts>...>{
            std::in_place,
            std::forward<T>(s0),
            std::forward<Ts>(ss)...
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct sender_traits
{
    static constexpr auto sender_types = meta::list<Ts...>{};
    static constexpr auto indices = meta::iota<sizeof ... (Ts)>;

    template <typename R>
    struct with
    {
        static constexpr auto receiver_type = meta::atom<R>{};
        static constexpr auto operation_type = meta::atom<operation<R, Ts...>>{};
        static constexpr auto receiver_types = meta::transform(
            indices,
            [] (auto index) {
                return meta::atom<receiver<index, R, Ts...>>{};
            });

        static constexpr auto value_types = traits::sender_values(
            meta::last(sender_types),
            meta::last(receiver_types)
        );

        static constexpr auto error_types = meta::zip_transform_unique(
            sender_types, receiver_types, traits::sender_errors);
    };
};

}   // namespace sequence_impl

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename ... Ts>
struct sender_traits<sequence_impl::sender<T, Ts...>>
    : sequence_impl::sender_traits<T, Ts...>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto sequence = sequence_impl::sequence{};

}   // namespace execution
