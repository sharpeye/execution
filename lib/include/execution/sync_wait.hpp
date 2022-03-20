#pragma once

#include "null_receiver.hpp"
#include "pipeable.hpp"
#include "sender_traits.hpp"
#include "tuple.hpp"

#include <exception>
#include <future>
#include <optional>

namespace this_thread {
namespace sync_wait_impl {

using namespace execution;

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct receiver
{
    std::promise<T>& promise;

    template <typename ... Ts>
    void set_value(Ts&& ... values)
    {
        promise.set_value(decayed_tuple_t<Ts...>{std::forward<Ts>(values)...});
    }

    void set_error(std::exception_ptr ex)
    {
        promise.set_exception(std::move(ex));
    }

    template <typename E>
    void set_error(E&& error)
    {
        promise.set_exception(std::make_exception_ptr(std::forward<E>(error)));
    }

    void set_stopped()
    {
        promise.set_value(std::nullopt);
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename ... Ts>
auto do_sync_wait(T&& sender, meta::atom<signature<Ts...>> expected_result_type)
{
    using result_t = std::optional<decayed_tuple_t<Ts...>>;

    constexpr auto sender_type = meta::atom<std::decay_t<T>>{};
    constexpr auto receiver_type = meta::atom<receiver<result_t>>{};
    constexpr auto value_types = traits::sender_values(
        sender_type,
        receiver_type);

    static_assert(meta::contains(value_types, expected_result_type));

    std::promise<result_t> promise;
    auto future = promise.get_future();

    auto op = connect(std::forward<T>(sender), receiver<result_t>{promise});
    start(op);

    future.wait();
    return future.get();
}

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct sync_wait_r
{
    auto operator () () const
    {
        return pipeable(*this);
    }

    template <typename T>
    auto operator () (T&& sender) const
    {
        return do_sync_wait(
            std::forward<T>(sender),
            meta::atom<signature<Ts...>>{}
        );
    }
};

///////////////////////////////////////////////////////////////////////////////

struct sync_wait
{
    auto operator () () const
    {
        return pipeable(*this);
    }

    template <typename T>
    auto operator () (T&& sender) const
    {
        constexpr auto sender_type = meta::atom<std::decay_t<T>>{};
        constexpr auto receiver_type = meta::atom<null_receiver>{};
        constexpr auto value_types = traits::sender_values(
            sender_type,
            receiver_type);

        static_assert(value_types.length == 1);

        return do_sync_wait(std::forward<T>(sender), value_types.head);
    }
};

}   // namespace sync_wait_impl

///////////////////////////////////////////////////////////////////////////////

template <typename ... Rs>
constexpr auto sync_wait_r = sync_wait_impl::sync_wait_r<Rs...>{};

constexpr auto sync_wait = sync_wait_impl::sync_wait{};

}   // namespace this_thread
