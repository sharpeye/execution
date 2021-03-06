#pragma once

#include "customization.hpp"
#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "tuple.hpp"
#include "variant.hpp"

#include <functional>
#include <tuple>

namespace execution {
namespace let_value_impl {

template <typename P, typename F, typename R>
struct operation;

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct forward_receiver
{
    operation<P, F, R>* _operation;

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
    friend auto tag_invoke(Tag tag, const forward_receiver<P, F, R>& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return tag(self._operation->get_receiver(), std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct operation
{
    using forward_receiver_t = forward_receiver<P, F, R>;

    static constexpr auto predecessor_type = meta::atom<P>{};
    static constexpr auto factory_type = meta::atom<F>{};
    static constexpr auto receiver_type = meta::atom<R>{};

    static constexpr auto predecessor_receiver_type = meta::atom<forward_receiver_t>{};
    static constexpr auto predecessor_operation_type = traits::sender_operation(
        predecessor_type,
        predecessor_receiver_type);
    static constexpr auto predecessor_value_types = traits::sender_values(
        predecessor_type,
        predecessor_receiver_type);

    static constexpr auto get_invoke_result =
        [] <typename ... Ts> (meta::atom<signature<Ts...>>) {
            return meta::atom<std::decay_t<decltype(
                std::apply(
                    std::declval<F>(),
                    std::declval<decayed_tuple_t<Ts...>&>()
                )
            )>>{};
        };

    static constexpr auto successor_types = meta::transform_unique(
        traits::sender_values(predecessor_type, receiver_type),
        get_invoke_result
    );

    static constexpr auto successor_operation_types =
        meta::transform(successor_types, [] (auto s) {
            return traits::sender_operation(s, receiver_type);
        });

    static constexpr auto as_tuple = [] <typename ... Ts> (meta::atom<signature<Ts...>>) {
        return meta::atom<decayed_tuple_t<Ts...>>{};
    };

    using state_t = variant_t<decltype(
          predecessor_type
        | predecessor_operation_type
        | successor_operation_types
    )>;

    using values_t = variant_t<decltype(
          meta::atom<std::monostate>{}
        | meta::transform(predecessor_value_types, as_tuple)
    )>;

    F _factory;
    R _receiver;
    values_t _values;
    state_t _state;

    template <typename Px, typename Fx, typename Rx>
    operation(Px&& predecessor, Fx&& factory, Rx&& receiver)
        : _factory(std::forward<Fx>(factory))
        , _receiver(std::forward<Rx>(receiver))
        , _state(std::in_place_type<P>, std::forward<Px>(predecessor))
    {}

    void start()
    {
        using operation_t = typename decltype(predecessor_operation_type)::type;

        auto& op = _state.template emplace<operation_t>(
            execution::connect(
                std::get<P>(std::move(_state)),
                forward_receiver_t{this}
            ));

        execution::start(op);
    }

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        using tuple_t = decayed_tuple_t<Ts...>;
        auto& tuple = _values.template emplace<tuple_t>(
            std::forward<Ts>(values)...);

        using successor_t = std::decay_t<decltype(std::apply(std::move(_factory), tuple))>;
        using operation_t = typename execution::sender_traits<successor_t, R>::operation_t;

        auto& op = _state.template emplace<operation_t>(execution::connect(
            std::apply(std::move(_factory), tuple),
            std::move(_receiver)
        ));

        execution::start(op);
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

    auto const& get_receiver() const noexcept
    {
        return _receiver;
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct sender
{
    P _predecessor;
    F _successor_factory;

    template <typename U, typename V>
    sender(U&& p, V&& func)
        : _predecessor(std::forward<U>(p))
        , _successor_factory(std::forward<V>(func))
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<P, F, R>{
            _predecessor,
            _successor_factory,
            std::forward<R>(receiver)
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<P, F, R>{
            std::move(_predecessor),
            std::move(_successor_factory),
            std::forward<R>(receiver)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct let_value
{
    template <typename F>
    constexpr auto operator () (F&& factory) const
    {
        return pipeable(*this, std::forward<F>(factory));
    }

    template <typename P, typename F>
    constexpr auto operator () (P&& predecessor, F&& factory) const
    {
        return sender<std::decay_t<P>, std::decay_t<F>>{
            std::forward<P>(predecessor),
            std::forward<F>(factory)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct sender_traits
{
    using operation_t = operation<P, F, R>;

    static constexpr auto predecessor_type = meta::atom<P>{};
    static constexpr auto factory_type = meta::atom<F>{};

    static constexpr auto receiver_type = meta::atom<R>{};

    static constexpr auto successors_value_types = meta::transform(
        operation_t::successor_types, [] (auto s) {
            return traits::sender_values(s, receiver_type);
        });

    static constexpr auto successors_error_types = meta::transform(
        operation_t::successor_types, [] (auto s) {
            return traits::sender_errors(s, receiver_type);
        });

    static constexpr auto value_types = meta::chain_unique(
        successors_value_types
    );

    static constexpr bool is_nothrow = meta::is_all(
        traits::sender_values(predecessor_type, receiver_type),
        [] (auto values) {
            return traits::is_nothrow_invocable(factory_type, values);
        });

    static constexpr auto predecessor_receiver_type = meta::atom<
        forward_receiver<P, F, R>>{};

    static constexpr auto error_types = meta::concat_unique(
        traits::sender_errors(predecessor_type, predecessor_receiver_type),
        successors_error_types,
        [] {
            if constexpr (is_nothrow) {
                return meta::list<>{};
            } else {
                return meta::list<std::exception_ptr>{};
            }
        } ());

    using errors_t = decltype(error_types);
    using values_t = decltype(value_types);
};

}   // namespace let_value_impl

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct sender_traits<let_value_impl::sender<P, F>, R>
    : let_value_impl::sender_traits<P, F, R>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto let_value = let_value_impl::let_value{};

}   // namespace execution
