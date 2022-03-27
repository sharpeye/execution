#pragma once

#include "sender_traits.hpp"
#include "stop_token.hpp"
#include "tuple.hpp"
#include "variant.hpp"

#include <atomic>
#include <exception>
#include <functional>
#include <optional>

namespace execution {

namespace when_all_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename T, auto index>
struct receiver
{
    T* _operation;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        _operation->set_value(index, std::forward<Ts>(values)...);
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
    friend auto tag_invoke(Tag tag, const receiver<T, index>& self, Ts&& ... args)
    {
        return tag(self._operation->get_receiver(), std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename ... Ts>
struct operation
{
    using self_t = operation<R, Ts...>;

    static constexpr auto indices = meta::iota<sizeof ... (Ts)>;
    static constexpr auto sender_types = meta::list<Ts...>{};
    static constexpr auto receiver_types = meta::transform(
        indices,
        [] (auto index) {
            return meta::atom<receiver<operation, index>>{};
        });

    static_assert(meta::is_all(indices, [] (auto index) {
        return traits::sender_values(
            sender_types[index],
            receiver_types[index]
        ).length <= 1;
    }));

    static constexpr auto operation_types = meta::zip_transform(
        sender_types, receiver_types,
        [] (auto s, auto r) {
            return traits::sender_operation(s, r);
        });

    static constexpr auto error_types = meta::zip_transform_unique(
        sender_types, receiver_types,
        [] (auto s, auto r) {
            return traits::sender_errors(s, r);
        });

    static constexpr auto value_types = meta::list<>{} |
        to_signature(meta::zip_transform(sender_types, receiver_types,
            [] (auto s, auto r) {
                return to_list(traits::sender_values(s, r).head);
            }
        ));

    template <typename ... Us>
    static consteval auto as_tuple(meta::atom<signature<Us...>>)
    {
        return meta::list<std::tuple<Us...>>{};
    }

    using storage_t = tuple_t<
        meta::zip_transform(sender_types, receiver_types, [] (auto s, auto r) {
            return meta::atom<
                variant_t<decltype(s | as_tuple(traits::sender_values(s, r).head))>
            >{};
        })>;

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
        tuple_t<operation_types> _operations;
        std::atomic<int> _active_ops = operation_types.length;

        std::stop_source _stop_source;
        std::stop_callback<cancel_callback> _stop_callback;

        operations(auto&& token, cancel_callback cb, auto&& ... ops)
            : _operations{std::move(ops)...}
            , _stop_callback{std::move(token), cb}
        {}
    };

    using state_t = std::variant<std::monostate, operations>;

    R _receiver;

    state_t _state;
    storage_t _storage;

    std::atomic_flag _error_or_stopped = ATOMIC_FLAG_INIT;
    std::optional<variant_t<decltype(error_types)>> _error;

    template <typename Rx, typename S>
    operation(Rx&& receiver, S&& senders)
        : _receiver(std::forward<Rx>(receiver))
        , _storage(std::forward<S>(senders))
    {}

    void start() &
    {
        start_impl(std::make_index_sequence<sender_types.length>{});
    }

    template <std::size_t ... Is>
    void start_impl(std::index_sequence<Is...>)
    {
        auto& ops = _state.template emplace<operations>(
            execution::get_stop_token(_receiver),
            cancel_callback{this},
            execution::connect(
                std::get<0>(std::get<Is>(std::move(_storage))),
                receiver<self_t, meta::index_t<Is>{}>{this})...
        );

        (execution::start(std::get<Is>(ops._operations)), ...);
    }

    template <int i, typename ... Us>
    void set_value(meta::index_t<i>, Us&& ... values)
    {
        using tuple_t = decayed_tuple_t<Us...>;

        std::get<i>(_storage).template emplace<tuple_t>(std::forward<Us>(values)...);

        notify_operation_complete();
    }

    template <typename E>
    void set_error(E&& error)
    {
        if (!_error_or_stopped.test_and_set()) {
            _error = std::forward<E>(error);

            cancel();
        }

        notify_operation_complete();
    }

    void set_stopped()
    {
        if (!_error_or_stopped.test_and_set()) {
            cancel();
        }

        notify_operation_complete();
    }

    void cancel()
    {
       get_operations()._stop_source.request_stop();
    }

    void notify_operation_complete()
    {
        if (get_operations()._active_ops.fetch_sub(1) == 1) {
            finish();
        }
    }

    void finish() noexcept
    {
        // reset callback
        _state = std::monostate{};

        if (_error_or_stopped.test()) {
            if (_error) {
                std::visit([this] (auto&& error) {
                    execution::set_error(std::move(_receiver), std::move(error));
                }, std::move(*_error));
                return;
            }
            
            execution::set_stopped(std::move(_receiver));
            return;
        }

        try {
            std::apply([this] (auto&&... tuples) {
                std::apply([this] (auto&& ... values) {
                        execution::set_value(
                            std::move(_receiver),
                            std::move(values)...
                        );
                    },
                    std::tuple_cat(std::get<1>(std::move(tuples))...)
                );
            }, std::move(_storage));
        } catch(...) {
            execution::set_error(std::move(_receiver), std::current_exception());
        }
    }

    auto const& get_receiver() const noexcept
    {
        return _receiver;
    }

    auto& get_operations()
    {
        return std::get<1>(_state);
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

struct when_all
{
    template <typename T>
    constexpr auto operator () (T&& s) const
    {
        return std::forward<T>(s);
    }

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
        using operation_t = operation<R, Ts...>;

        static constexpr auto receiver_type = meta::atom<R>{};
        static constexpr auto operation_type = meta::atom<operation_t>{};
        static constexpr auto receiver_types = meta::transform(
            indices,
            [] (auto index) {
                return meta::atom<receiver<operation_t, index>>{};
            });

        static constexpr auto value_types = meta::list<>{} |
            to_signature(meta::zip_transform(sender_types, receiver_types,
                [] (auto s, auto r) {
                    return to_list(traits::sender_values(s, r).head);
                }
            ));

        static constexpr auto error_types = meta::zip_transform_unique(
            sender_types, receiver_types, [] (auto s, auto r) {
                return traits::sender_errors(s, r);
            });
    };
};

}   // namespace when_all_impl

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct sender_traits<when_all_impl::sender<Ts...>>
    : when_all_impl::sender_traits<Ts...>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto when_all = when_all_impl::when_all{};

}   // namespace execution
