#pragma once

#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "stop_token.hpp"
#include "tuple.hpp"
#include "variant.hpp"

namespace execution {
namespace finally_impl {

template <typename S, typename C, typename R>
struct operation;

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename C, typename R>
struct source_receiver
{
    operation<S, C, R>* _operation;

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
    friend auto tag_invoke(Tag tag, const source_receiver<S, C, R>& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return tag(self._operation->get_receiver(), std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename C, typename R>
struct completion_receiver
{
    operation<S, C, R>* _operation;

    void set_value()
    {
        _operation->finish();
    }

    template <typename E>
    void set_error(E&& error)
    {
        _operation->completion_error(std::forward<E>(error));
    }

    void set_stopped()
    {
        _operation->completion_stopped();
    }

    // tag_invoke

    template <typename Tag, typename ... Ts>
    friend auto tag_invoke(Tag tag, const completion_receiver<S, C, R>& self, Ts&& ... args)
        noexcept(is_nothrow_tag_invocable_v<Tag, R, Ts...>)
        -> tag_invoke_result_t<Tag, R, Ts...>
    {
        return tag(self._operation->get_receiver(), std::forward<Ts>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename C, typename R>
struct operation
{
    using source_receiver_t = source_receiver<S, C, R>;
    using completion_receiver_t = completion_receiver<S, C, R>;

    static constexpr auto source_type = meta::atom<S>{};
    static constexpr auto completion_type = meta::atom<C>{};
    static constexpr auto receiver_type = meta::atom<R>{};

    static constexpr auto source_receiver_type = meta::atom<source_receiver_t>{};
    static constexpr auto completion_receiver_type = meta::atom<completion_receiver_t>{};

    static_assert(traits::sender_values(completion_type, completion_receiver_type)
        == meta::list<signature<>>{});

    static constexpr auto source_operation_type = traits::sender_operation(
        source_type,
        source_receiver_type);

    static constexpr auto completion_operation_type = traits::sender_operation(
        completion_type,
        completion_receiver_type);

    struct init_tag {};
    struct stopped_tag {};
    struct values_tag {};
    struct error_tag {};

    static constexpr auto value_types = meta::transform(
        traits::sender_values(source_type, source_receiver_type),
        []<typename ... Ts>(meta::atom<signature<Ts...>>) {
            return meta::atom<decayed_tuple_t<values_tag, Ts...>>{};
        }
    );

    static constexpr auto error_types = meta::transform(
        meta::concat_unique(
            meta::list<std::exception_ptr>{},
            traits::sender_errors(source_type, source_receiver_type),
            traits::sender_errors(completion_type, completion_receiver_type)
        ),
        []<typename E>(meta::atom<E>) {
            return meta::atom<std::tuple<error_tag, E>>{};
        }
    );

    using state_t = variant_t<decltype(
          meta::atom<std::monostate>{}
        | source_operation_type
        | completion_operation_type
    )>;

    using storage_t = variant_t<decltype(
          meta::atom<std::tuple<init_tag, S, C>>{}
        | meta::atom<std::tuple<stopped_tag>>{}
        | value_types
        | error_types
    )>;

    R _receiver;
    state_t _state;
    storage_t _storage;

    template <typename R1, typename S1, typename C1>
    operation(R1&& receiver, S1&& source, C1&& completion)
        : _receiver{std::forward<R1>(receiver)}
        , _state{}
        , _storage{
            std::in_place_index<0>,
            init_tag{},
            std::forward<S1>(source),
            std::forward<C1>(completion)
        }
    {}

    void start() & noexcept
    {
        auto&& source = std::get<1>(std::get<0>(std::move(_storage)));

        auto& op = _state.template emplace<1>(
            execution::connect(std::move(source), source_receiver_t{this})
        );

        execution::start(op);
    }

    void start_completion(C&& completion)
    {
        auto& op = _state.template emplace<2>(
            execution::connect(std::move(completion), completion_receiver_t{this})
        );

        execution::start(op);
    }

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        using tuple_t = decayed_tuple_t<values_tag, Ts...>;

        auto completion = std::get<2>(std::get<0>(std::move(_storage)));

        _storage.template emplace<tuple_t>(
            values_tag{},
            std::forward<Ts>(values)...
        );

        start_completion(std::move(completion));
    }

    template <typename E>
    void set_error(E&& error)
    {
        using tuple_t = decayed_tuple_t<error_tag, E>;

        auto completion = std::get<2>(std::get<0>(std::move(_storage)));

        _storage.template emplace<tuple_t>(
            error_tag{},
            std::forward<E>(error)
        );

        start_completion(std::move(completion));
    }

    void set_stopped()
    {
        auto completion = std::get<2>(std::get<0>(std::move(_storage)));

        _storage.template emplace<std::tuple<stopped_tag>>();

        start_completion(std::move(completion));
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

    template <typename E>
    void completion_error(E&& error)
    {
        execution::set_error(std::move(_receiver), std::forward<E>(error));
    }

    void completion_stopped()
    {
        execution::set_stopped(std::move(_receiver));
    }

    void finish() noexcept
    {
        try {
            std::visit([this] (auto&& tuple) {
                std::apply(
                    [this] (auto&& ... values) {
                        set_by_tag(std::move(values)...);
                    },
                    std::move(tuple)
                );
            }, std::move(_storage));
        } catch (...) {
            execution::set_error(std::move(_receiver), std::current_exception());
        }
    }

    auto const& get_receiver() const noexcept
    {
        return _receiver;
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename C>
struct sender
{
    S _source;
    C _completion;

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<S, C, R>{
            std::forward<R>(receiver),
            _source,
            _completion
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<S, C, R>{
            std::forward<R>(receiver),
            std::move(_source),
            std::move(_completion)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct finally
{
    template <typename C>
    constexpr auto operator () (C&& completion) const
    {
        return pipeable(*this, std::forward<C>(completion));
    }

    template <typename S, typename C>
    constexpr auto operator () (S&& source, C&& completion) const
    {
        return sender<std::decay_t<S>, std::decay_t<C>>{
            std::forward<S>(source),
            std::forward<C>(completion)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename C, typename R>
struct sender_traits
{
    using source_receiver_t = source_receiver<S, C, R>;
    using completion_receiver_t = completion_receiver<S, C, R>;

    static constexpr auto source_type = meta::atom<S>{};
    static constexpr auto completion_type = meta::atom<C>{};
    static constexpr auto receiver_type = meta::atom<R>{};

    static constexpr auto source_receiver_type = meta::atom<source_receiver_t>{};
    static constexpr auto completion_receiver_type = meta::atom<completion_receiver_t>{};

    using operation_t = operation<S, C, R>;

    using errors_t = decltype(meta::unique(
          meta::list<std::exception_ptr>{}
        | traits::sender_errors(source_type, source_receiver_type)
        | traits::sender_errors(completion_type, completion_receiver_type)
    ));

    using values_t = decltype(
        traits::sender_values(source_type, source_receiver_type));
};

}   // namespace finally_impl

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename C, typename R>
struct sender_traits<finally_impl::sender<S, C>, R>
    : finally_impl::sender_traits<S, C, R>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto finally = finally_impl::finally{};

}   // namespace execution
