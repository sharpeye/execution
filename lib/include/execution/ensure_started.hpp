#pragma once

#include "null_receiver.hpp"
#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "stop_token.hpp"
#include "tuple.hpp"
#include "variant.hpp"

#include <atomic>
#include <memory>
#include <optional>
#include <utility>

namespace execution {
namespace ensure_started_impl {

////////////////////////////////////////////////////////////////////////////////

struct init_tag {};
struct stopped_tag {};
struct error_tag {};
struct values_tag {};

template <typename T>
struct shared_state
{
    T _storage;
    std::stop_source _stop_source;
    std::atomic_flag _flag = {};

    void* _obj = nullptr;
    void (*_finish)(void*) = nullptr;

    // TODO: unique_ptr
    std::shared_ptr<void> _operation;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct receiver
{
    std::shared_ptr<T> _state;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        std::shared_ptr<T> state = std::move(_state);

        state->_storage.template emplace<std::tuple<values_tag, std::decay_t<Ts>...>>(
            values_tag{}, std::forward<Ts>(values)...);

        if (state->_flag.test_and_set()) {
            state->_finish(state->_obj);
        }
    }

    void set_stopped()
    {
        std::shared_ptr<T> state = std::move(_state);

        state->_storage = std::tuple<stopped_tag>{};

        if (state->_flag.test_and_set()) {
            state->_finish(state->_obj);
        }
    }

    template <typename E>
    void set_error(E&& error)
    {
        std::shared_ptr<T> state = std::move(_state);

        state->_storage.template emplace<std::tuple<error_tag, std::decay_t<E>>>(
            error_tag{}, std::forward<E>(error));

        if (state->_flag.test_and_set()) {
            state->_finish(state->_obj);
        }
    }

    // tag_invoke

    friend auto tag_invoke(tag_t<get_stop_token>, const receiver<T>& self) noexcept
    {
        return self._state->_stop_source.get_token();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename T>
struct operation
{
    struct cancel_callback
    {
        operation<R, T>* _this;

        void operator () () noexcept
        {
            _this->cancel();
        }
    };

    R _receiver;
    std::shared_ptr<T> _state;
    std::optional<std::stop_callback<cancel_callback>> _stop_callback;

    template <typename U>
    operation(U&& receiver, std::shared_ptr<T>&& state)
        : _receiver(std::forward<U>(receiver))
        , _state(std::move(state))
    {}

    ~operation()
    {
        if (_state) {
            _state->_stop_source.request_stop();
        }
    }

    void start() & noexcept
    {
        _stop_callback.emplace(
            execution::get_stop_token(_receiver),
            cancel_callback{this});

        _state->_obj = this;
        _state->_finish = [] (void* obj) {
            static_cast<operation<R, T>*>(obj)->finish();
        };

        if (_state->_flag.test_and_set()) {
            finish();
        }
    }

    void finish()
    {
        _stop_callback.reset();
        std::shared_ptr<T> state = std::move(_state);

        try {
            std::visit([this] (auto&& tuple) {
                std::apply(
                    [this] (auto&& ... values) {
                        set_by_tag(std::move(values)...);
                    },
                    std::move(tuple)
                );
            }, std::move(state->_storage));
        } catch (...) {
            execution::set_error(std::move(_receiver), std::current_exception());
        }
    }

    template <typename E>
    void set_by_tag(error_tag, E&& error)
    {
        execution::set_error(std::move(_receiver), std::move(error));
    }

    template <typename ... Ts>
    void set_by_tag(values_tag, Ts&& ... values)
    {
        execution::set_value(std::move(_receiver), std::move(values)...);
    }

    void set_by_tag(stopped_tag)
    {
        execution::set_stopped(std::move(_receiver));
    }

    template<typename ... Ts>
    [[ noreturn ]] void set_by_tag(init_tag, Ts&&...)
    {
        std::terminate();
    }

    void cancel()
    {
        _state->_stop_source.request_stop();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename T>
struct sender
{
    using source_t = S;

    std::shared_ptr<T> _state;

    sender(sender const& other) = delete;
    sender(sender&& other) = default;

    sender& operator = (sender const& other) = delete;
    sender& operator = (sender&& other) = default;

    explicit sender(std::shared_ptr<T>&& state)
        : _state{std::move(state)}
    {}

    ~sender()
    {
        if (_state) {
            _state->_stop_source.request_stop();
        }
    }

    template <typename R>
    auto connect(R&& receiver) && -> operation<R, T>
    {
        return { std::forward<R>(receiver), std::move(_state) };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct ensure_started
{
    auto operator () () const
    {
        return pipeable(*this);
    }

    template <typename S>
    constexpr auto operator () (S&& source) const
    {
        using source_t = std::decay_t<S>;

        constexpr auto source_type = meta::atom<source_t>{};
        constexpr auto source_value_types = traits::sender_values(
            source_type, meta::atom<null_receiver>{});
        constexpr auto source_error_types = traits::sender_errors(
            source_type,
            meta::atom<null_receiver>{});

        constexpr auto value_types = meta::transform(
            source_value_types,
            []<typename ... Ts>(meta::atom<signature<Ts...>>) {
                return meta::atom<std::tuple<values_tag, Ts...>>{};
            }
        );

        constexpr auto error_types = meta::transform(
            source_error_types,
            []<typename E>(meta::atom<E>) {
                return meta::atom<std::tuple<error_tag, E>>{};
            }
        );

        using storage_t = variant_t<decltype(
              meta::atom<std::tuple<init_tag>>{}
            | meta::atom<std::tuple<stopped_tag>>{}
            | value_types
            | error_types
        )>;

        using state_t = shared_state<storage_t>;

        using operation_t = typename sender_traits<source_t, receiver<state_t>>
            ::operation_t;

        auto state = std::make_shared<state_t>();
        auto op = std::make_shared<operation_t>(
            execution::connect(std::forward<S>(source), receiver<state_t>{state}));

        state->_operation = op;

        execution::start(*op);

        return sender<source_t, state_t> { std::move(state) };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename T>
struct sender_traits
{
    // TODO: get all data from storage_t
    static constexpr auto source_type = meta::atom<S>{};
    static constexpr auto receiver_type = meta::atom<R>{};
    static constexpr auto value_types = traits::sender_values(
        source_type, receiver_type);
    static constexpr auto error_types = meta::concat_unique(
        traits::sender_errors(source_type, receiver_type),
        meta::list<std::exception_ptr>{}
    );

    using operation_t = operation<R, T>;
    using errors_t = decltype(error_types);
    using values_t = decltype(value_types);
};

}   // namespace ensure_started_impl

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename S, typename T>
struct sender_traits<ensure_started_impl::sender<S, T>, R>
    : ensure_started_impl::sender_traits<R, S, T>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto ensure_started = ensure_started_impl::ensure_started {};

}   // namespace execution
