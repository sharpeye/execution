#pragma once

#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "tuple.hpp"
#include "variant.hpp"

#include <functional>
#include <tuple>

namespace execution {
namespace let_value_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct forward_receiver
{
    T* _operation;

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

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, const forward_receiver<T>& self, Ts&& ... args)
    {
        return tag(self._operation->get_receiver(), std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct operation
{
    using self_t = operation<P, F, R>;

    static constexpr auto predecessor_type = meta::atom<P>{};
    static constexpr auto factory_type = meta::atom<F>{};
    static constexpr auto receiver_type = meta::atom<R>{};

    static constexpr auto predecessor_receiver_type = meta::atom<forward_receiver<self_t>>{};
    static constexpr auto predecessor_operation_type = traits::sender_operation(
        predecessor_type,
        predecessor_receiver_type);
    static constexpr auto predecessor_value_types = traits::sender_values(
        predecessor_type,
        predecessor_receiver_type);

    static constexpr auto successor_types = meta::transform(
        predecessor_value_types,
        [] (auto sig) {
            return traits::invoke_result(factory_type, sig);
        });

    using state_t = variant_t<
          predecessor_type
        | predecessor_operation_type
        | meta::transform(successor_types, [] (auto s) {
            return traits::sender_operation(s, receiver_type);
        })
    >;

    using values_t = variant_t<
          meta::atom<std::monostate>{}
        | meta::transform(predecessor_value_types, meta::convert_to<decayed_tuple_t>)
    >;

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
        using receiver_t = typename decltype(predecessor_receiver_type)::type;

        auto& op = _state.template emplace<operation_t>(
            execution::connect(
                std::get<P>(std::move(_state)),
                receiver_t{this}
            ));

        execution::start(op);
    }

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        using tuple_t = decayed_tuple_t<Ts...>;
        using operation_t = typename decltype(
            traits::sender_operation(
                traits::invoke_result(factory_type, meta::atom<signature<Ts...>>{}),
                receiver_type
            ))::type;

        auto& tuple = _values.template emplace<tuple_t>(
            std::forward<Ts>(values)...);

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

template <typename P, typename F>
struct sender_traits
{
    static constexpr auto predecessor_type = meta::atom<P>{};
    static constexpr auto factory_type = meta::atom<F>{};

    template <typename R>
    struct with
    {
        static constexpr auto receiver_type = meta::atom<R>{};
        static constexpr auto operation_type = meta::atom<operation<P, F, R>>{};

        static constexpr auto predecessor_receiver_type = meta::atom<
            forward_receiver<operation<P, F, R>>>{};

        static constexpr auto successor_types = meta::transform_unique(
            traits::sender_values(predecessor_type, receiver_type),
            [] (auto sig) {
                return traits::invoke_result(factory_type, sig);
            }
        );

        static constexpr auto value_types = meta::chain_unique(
            meta::transform(
                successor_types,
                [] (auto s) {
                    return traits::sender_values(s, receiver_type);
                }
            ));

        static constexpr auto error_types = meta::concat_unique(
            traits::sender_errors(predecessor_type, predecessor_receiver_type),
            meta::transform(successor_types, [] (auto s) {
                return traits::sender_errors(s, receiver_type);
            }),
            [] {
                constexpr bool nothrow = meta::is_all(
                    traits::sender_values(predecessor_type, receiver_type),
                    [] (auto sig) {
                        return traits::is_nothrow_invocable(factory_type, sig);
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

}   // namespace let_value_impl

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct sender_traits<let_value_impl::sender<P, F>>
    : let_value_impl::sender_traits<P, F>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto let_value = let_value_impl::let_value{};

}   // namespace execution
