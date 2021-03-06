#pragma once

#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "stop_token.hpp"
#include "tuple.hpp"
#include "variant.hpp"

#include <atomic>

namespace execution {
namespace stop_when_impl {

template <typename S, typename T, typename R>
class operation;

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename T, typename R>
struct source_receiver
{
    operation<S, T, R>* _operation;

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

    friend auto tag_invoke(tag_t<get_stop_token>, const source_receiver<S, T, R>& self) noexcept
    {
        return self._operation->get_stop_token();
    }

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, const source_receiver<S, T, R>& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return tag(self._operation->get_receiver(), std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename T, typename R>
struct trigger_receiver
{
    operation<S, T, R>* _operation;

    template <typename ... Ts>
    void set_value(Ts&& ...)
    {
        _operation->notify_operation_complete();
    }

    template <typename E>
    void set_error(E&&)
    {
        _operation->notify_operation_complete();
    }

    void set_stopped()
    {
        _operation->notify_operation_complete();
    }

    // tag_invoke

    friend auto tag_invoke(tag_t<get_stop_token>, const trigger_receiver<S, T, R>& self) noexcept
    {
        return self._operation->get_stop_token();
    }

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, const trigger_receiver<S, T, R>& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return tag(self._operation->get_receiver(), std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename T, typename R>
class operation
{
    using source_receiver_t = source_receiver<S, T, R>;
    using trigger_receiver_t = trigger_receiver<S, T, R>;

    static constexpr auto source_type = meta::atom<S>{};
    static constexpr auto trigger_type = meta::atom<T>{};

    static constexpr auto source_receiver_type = meta::atom<source_receiver_t>{};
    static constexpr auto trigger_receiver_type = meta::atom<trigger_receiver_t>{};

    static constexpr auto source_operation_type = traits::sender_operation(
        source_type, source_receiver_type);

    static constexpr auto trigger_operation_type = traits::sender_operation(
        trigger_type, trigger_receiver_type);

    using source_operation_t = typename decltype(source_operation_type)::type;
    using trigger_operation_t = typename decltype(trigger_operation_type)::type;

    struct cancel_callback
    {
        operation* _operation;

        void operator ()() noexcept
        {
            _operation->cancel();
        }
    };

    struct operations
    {
        source_operation_t _source;
        trigger_operation_t _trigger;
        std::stop_callback<cancel_callback> _stop_callback;

        std::stop_source _stop_source;
        std::atomic<int> _active_ops = 2;

        operations(auto&& src, auto&& tgr, auto&& token, cancel_callback cb)
            : _source{std::forward<decltype(src)>(src)}
            , _trigger{std::forward<decltype(tgr)>(tgr)}
            , _stop_callback{std::forward<decltype(token)>(token), cb}
        {}
    };

    using state_t = std::variant<std::monostate, operations>;

    static constexpr auto source_value_types = traits::sender_values(
        source_type,
        source_receiver_type);

    static constexpr auto source_error_types = traits::sender_errors(
        source_type,
        source_receiver_type);

    static constexpr auto value_types = meta::transform(
        source_value_types,
        []<typename ... Ts>(meta::atom<signature<Ts...>>) {
            return meta::atom<decayed_tuple_t<set_value_fn, Ts...>>{};
        }
    );

    static constexpr auto error_types = meta::transform(
        source_error_types,
        []<typename E>(meta::atom<E>) {
            return meta::atom<std::tuple<set_error_fn, E>>{};
        }
    );

    using storage_t = variant_t<decltype(
          meta::atom<std::tuple<std::monostate, S, T>>{}
        | meta::atom<std::tuple<set_stopped_fn>>{}
        | value_types
        | error_types
    )>;

private:
    R _receiver;

    state_t _state;
    storage_t _storage;

public:
    template <typename Sx, typename Tx, typename Rx>
    operation(Sx&& src, Tx&& trigger, Rx&& receiver)
        : _receiver(std::forward<Rx>(receiver))
        , _storage{
            std::in_place_index<0>,
            std::monostate{},
            std::forward<Sx>(src),
            std::forward<Tx>(trigger)
        }
    {}

    void start() &
    {
        auto&& [tag, src, trigger] = std::get<0>(std::move(_storage));

        auto& ops = _state.template emplace<operations>(
            execution::connect(std::move(src), source_receiver_t{this}),
            execution::connect(std::move(trigger), trigger_receiver_t{this}),
            execution::get_stop_token(_receiver),
            cancel_callback{this}
        );

        execution::start(ops._source);
        execution::start(ops._trigger);
    }

public:
    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        using tuple_t = decayed_tuple_t<set_value_fn, Ts...>;

        _storage.template emplace<tuple_t>(
            execution::set_value,
            std::forward<Ts>(values)...
        );

        notify_operation_complete();
    }

    template <typename E>
    void set_error(E&& error)
    {
        using tuple_t = decayed_tuple_t<set_error_fn, E>;

        _storage.template emplace<tuple_t>(
            execution::set_error,
            std::forward<E>(error)
        );

        notify_operation_complete();
    }

    void set_stopped()
    {
        _storage = std::make_tuple(execution::set_stopped);

        notify_operation_complete();
    }

    void notify_operation_complete()
    {
        auto& ops = std::get<operations>(_state);

        ops._stop_source.request_stop();

        if (ops._active_ops.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            finish();
        }
    }

    void cancel()
    {
        auto& ops = std::get<operations>(_state);
        ops._stop_source.request_stop();
    }

    template <typename CPO, typename ... Ts>
    void invoke_cpo(CPO cpo, Ts&& ... values)
    {
        cpo(std::move(_receiver), std::move(values)...);
    }

    template<typename ... Ts>
    [[ noreturn ]] void invoke_cpo(std::monostate, Ts&&...)
    {
        std::terminate();
    }

    void finish() noexcept
    {
        // reset callback
        _state = std::monostate{};

        try {
            std::visit([this] (auto&& tuple) {
                std::apply(
                    [this] (auto&& ... values) {
                        invoke_cpo(std::move(values)...);
                    },
                    std::move(tuple)
                );
            }, std::move(_storage));
        } catch (...) {
            execution::set_error(std::move(_receiver), std::current_exception());
        }
    }

    auto get_stop_token() const noexcept {
        auto& ops = std::get<operations>(_state);
        return ops._stop_source.get_token();
    }

    auto const& get_receiver() const noexcept
    {
        return _receiver;
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename T>
struct sender
{
    S _source;
    T _trigger;

    template <typename U, typename V>
    sender(U&& src, V&& trigger)
        : _source(std::forward<U>(src))
        , _trigger(std::forward<V>(trigger))
    {}

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<S, T, R>{
            _source,
            _trigger,
            std::forward<R>(receiver)
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<S, T, R>{
            std::move(_source),
            std::move(_trigger),
            std::forward<R>(receiver)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct stop_when
{
    template <typename T>
    constexpr auto operator () (T&& trigger) const
    {
        return pipeable(*this, std::forward<T>(trigger));
    }

    template <typename T, typename U>
    constexpr auto operator () (T&& source, U&& trigger) const
    {
        return sender<std::decay_t<T>, std::decay_t<U>>{
            std::forward<T>(source),
            std::forward<U>(trigger)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename U, typename R>
struct sender_traits
{
    static constexpr auto source_type = meta::atom<T>{};
    static constexpr auto trigger_type = meta::atom<U>{};
    static constexpr auto receiver_type = meta::atom<R>{};

    static constexpr auto value_types = traits::sender_values(
        source_type, receiver_type);

    static constexpr auto error_types = meta::concat_unique(
        traits::sender_errors(source_type, receiver_type),
        meta::list<std::exception_ptr>{}
    );

    using operation_t = operation<T, U, R>;
    using errors_t = decltype(error_types);
    using values_t = decltype(value_types);
};

}   // namespace stop_when_impl

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename U, typename R>
struct sender_traits<stop_when_impl::sender<T, U>, R>
    : stop_when_impl::sender_traits<T, U, R>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto stop_when = stop_when_impl::stop_when{};

}   // namespace execution
