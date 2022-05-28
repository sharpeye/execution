#pragma once

#include "bulk.hpp"
#include "sender_traits.hpp"
#include "task_queue.hpp"
#include "tuple.hpp"
#include "variant.hpp"

#include <exception>
#include <functional>

namespace execution {
namespace thread_pool_bulk_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename R>
struct source_receiver
{
    T* _op;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        _op->set_value(std::forward<Ts>(values)...);
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
    friend auto tag_invoke(Tag tag, source_receiver<T, R> const& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return std::invoke(tag, self._op->_receiver, std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename R, typename S, typename I, typename F>
struct operation
    : task_base
{
    using source_receiver_t = source_receiver<operation, R>;

    static constexpr auto source_type = meta::atom<S>{};
    static constexpr auto source_receiver_type = meta::atom<source_receiver_t>{};
    static constexpr auto source_operation_type = traits::sender_operation(
        source_type, source_receiver_type);

    using source_operation_t = typename decltype(source_operation_type)::type;

    static constexpr auto source_value_types = traits::sender_values(
        source_type,
        source_receiver_type);

    static constexpr auto as_tuple = [] <typename ... Ts> (meta::atom<signature<Ts...>>) {
        return meta::atom<decayed_tuple_t<Ts...>>{};
    };

    struct cancel_callback
    {
        std::atomic_flag* _flag;

        void operator () () noexcept
        {
            _flag->test_and_set();
        }
    };

    struct bulk_state
    {
        std::atomic_flag _error_or_stopped = {};
        std::atomic<I> _active_ops = {};
        std::atomic<I> _index = {};

        std::exception_ptr _error;

        std::stop_callback<cancel_callback> _stop_callback;

        bulk_state(I shape, auto token)
            : _active_ops {shape}
            , _stop_callback {
                token,
                cancel_callback{&_error_or_stopped}
            }
        {}
    };

    using state_t = std::variant<
        std::monostate,
        source_operation_t,
        bulk_state
    >;

    using storage_t = variant_t<decltype(
          meta::atom<std::monostate>{}
        | source_type
        | meta::transform(source_value_types, as_tuple)
    )>;

    P* _pool;
    R _receiver;
    I _shape;
    F _func;

    state_t _state;
    storage_t _storage;

    template <typename T, typename X, typename U>
    operation(T&& receiver, P* pool, X&& source, I shape, U&& func)
        : task_base {nullptr}
        , _pool {pool}
        , _receiver {std::forward<T>(receiver)}
        , _shape(shape)
        , _func {std::forward<U>(func)}
        , _storage {std::in_place_type<S>, std::forward<X>(source)}
    {}

    void start() &
    {
        auto& op = _state.template emplace<source_operation_t>(
            execution::connect(
                std::get<S>(std::move(_storage)),
                source_receiver_t{this}
            ));

        _storage = std::monostate {};

        execution::start(op);
    }

    template <typename T>
    void execute()
    {
        auto& state = std::get<bulk_state>(_state);

        try {
            if (!state._error_or_stopped.test()) {
                const I i = state._index.fetch_add(1);

                std::apply(
                    [this, i] (auto&& ... values) {
                        std::invoke(_func, i, std::move(values)...);
                    },
                    std::get<T>(_storage)
                );
            }
        }
        catch (...) {
            if (!state._error_or_stopped.test_and_set()) {
                state._error = std::current_exception();
            }
        }

        if (state._active_ops.fetch_sub(1) == 1) {
            finish<T>(state);
        }
    }

    template <typename T>
    void finish(bulk_state& state)
    {
        if (!state._error_or_stopped.test()) {
            execution::apply_value(
                std::move(_receiver),
                std::get<T>(std::move(_storage))
            );
            return;
        }

        if (state._error) {
            execution::set_error(std::move(_receiver), std::move(state._error));
            return;
        }

        execution::set_stopped(std::move(_receiver));
    }

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        using tuple_t = decayed_tuple_t<Ts...>;

        _storage.template emplace<tuple_t>(std::forward<Ts>(values)...);

        auto& state = _state.template emplace<bulk_state>(
            _shape,
            execution::get_stop_token(_receiver));

        this->_execute = static_cast<task_base::execute_t>(
            &operation::execute<tuple_t>);

        for (I i = {}; i != _shape; ++i) {
            _pool->schedule(this);
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

template <typename P, typename S, typename I, typename F>
struct sender
{
    P* _pool;
    S _source;
    I _shape;
    F _func;

    template <typename R>
    operation<P, R, S, I, F> connect(R&& receiver) &
    {
        return {
            std::forward<R>(receiver),
            _pool,
            _source,
            _shape,
            _func
        };
    }

    template <typename R>
    operation<P, R, S, I, F> connect(R&& receiver) &&
    {
        return {
            std::forward<R>(receiver),
            _pool,
            std::move(_source),
            _shape,
            std::move(_func)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename P, typename S, typename I, typename F, typename R>
struct sender_traits
{
    using operation_t = operation<P, R, S, I, F>;
    using source_receiver_t = source_receiver<operation_t, R>;

    static constexpr auto source_type = meta::atom<S>{};
    static constexpr auto source_receiver_type = meta::atom<source_receiver_t>{};

    using errors_t = decltype(meta::concat_unique(
        traits::sender_errors(source_type, source_receiver_type),
        meta::list<std::exception_ptr>{}
    ));

    using values_t = typename execution::sender_traits<S, source_receiver_t>
        ::values_t;
};

}   // namespace thread_pool_bulk_impl

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename S, typename I, typename F, typename R>
struct sender_traits<thread_pool_bulk_impl::sender<T, S, I, F>, R>
    : thread_pool_bulk_impl::sender_traits<T, S, I, F, R>
{};

////////////////////////////////////////////////////////////////////////////////

}   // namespace execution
