#pragma once

#include "null_receiver.hpp"
#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "tuple.hpp"
#include "stop_token.hpp"

#include <exception>
#include <future>
#include <optional>
#include <system_error>

namespace this_thread {
namespace sync_wait_impl {

using namespace execution;

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct receiver
{
    std::promise<T>& _promise;
    std::stop_source _stop_source;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        _promise.set_value(decayed_tuple_t<Ts...>{std::forward<Ts>(values)...});
    }

    void set_error(std::exception_ptr ex)
    {
        _promise.set_exception(std::move(ex));
    }

    void set_error(std::error_code ec)
    {
        _promise.set_exception(std::make_exception_ptr(
            std::system_error{ec}));
    }

    template <typename E>
    void set_error(E&& error)
    {
        _promise.set_exception(std::make_exception_ptr(std::forward<E>(error)));
    }

    void set_stopped()
    {
        _promise.set_value(std::nullopt);
    }

    // tag_invoke

    friend auto tag_invoke(tag_t<get_stop_token>, const receiver<T>& self) noexcept
    {
        return self._stop_source.get_token();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename ... Ts>
auto do_sync_wait(
    T&& sender,
    std::stop_source ss,
    meta::atom<signature<Ts...>> expected_result_type)
{
    using result_t = std::optional<decayed_tuple_t<Ts...>>;

    constexpr auto sender_type = meta::atom<std::decay_t<T>>{};
    constexpr auto receiver_type = meta::atom<receiver<result_t>>{};
    constexpr auto value_types = traits::sender_values(
        sender_type,
        receiver_type);

    using operation_t = typename decltype(traits::sender_operation(
        sender_type, receiver_type))::type;

    static_assert(meta::contains(value_types, expected_result_type));

    std::promise<result_t> promise;
    auto future = promise.get_future();

    operation_t op {connect(
        std::forward<T>(sender),
        receiver<result_t>{promise, std::move(ss)}
    )};

    start(op);

    return future.get();
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct sync_wait_r
{
    auto operator () () const
    {
        return pipeable(*this);
    }

    auto operator () (std::stop_source ss) const
    {
        return pipeable(*this, std::move(ss));
    }

    template <typename T>
    auto operator () (T&& sender) const
    {
        return sync_wait_r{}(std::forward<T>(sender), std::stop_source{});
    }

    template <typename T>
    auto operator () (T&& sender, std::stop_source ss) const
    {
        return do_sync_wait(
            std::forward<T>(sender),
            std::move(ss),
            meta::atom<signature<Ts...>>{}
        );
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sync_wait
{
    auto operator () () const
    {
        return pipeable(*this);
    }

    auto operator () (std::stop_source ss) const
    {
        return pipeable(*this, std::move(ss));
    }

    template <typename T>
    auto operator () (T&& sender) const
    {
        return sync_wait{}(std::forward<T>(sender), std::stop_source{});
    }

    template <typename T>
    auto operator () (T&& sender, std::stop_source ss) const
    {
        constexpr auto sender_type = meta::atom<std::decay_t<T>>{};
        constexpr auto receiver_type = meta::atom<null_receiver>{};
        constexpr auto value_types = traits::sender_values(
            sender_type,
            receiver_type);

        static_assert(value_types.size == 1);

        return do_sync_wait(
            std::forward<T>(sender),
            std::move(ss),
            value_types.head
        );
    }
};

}   // namespace sync_wait_impl

////////////////////////////////////////////////////////////////////////////////

template <typename ... Rs>
constexpr auto sync_wait_r = sync_wait_impl::sync_wait_r<Rs...>{};

constexpr auto sync_wait = sync_wait_impl::sync_wait{};

}   // namespace this_thread
