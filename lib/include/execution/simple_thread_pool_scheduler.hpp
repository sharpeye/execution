#pragma once

#include "simple_thread_pool_bulk.hpp"

namespace execution {
namespace simple_thread_pool_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct task_impl
    : task_base
{
    task_impl()
        : task_base{ static_cast<execute_t>(&T::execute) }
    {}
};

template <typename R>
struct operation_base
    : task_impl<operation_base<R>>
{
    thread_pool* _pool;
    R _receiver;

    void execute()
    {
        if (execution::get_stop_token(_receiver).stop_requested()) {
            execution::set_stopped(std::move(_receiver));
        } else {
            execution::set_value(std::move(_receiver));
        }
    }

    void enqueue()
    {
        _pool->enqueue(this);
    }

    void enqueue(std::chrono::milliseconds dt)
    {
        _pool->enqueue(dt, this);
    }

    void enqueue(time_point_t dl)
    {
        _pool->enqueue(dl, this);
    }
};

template <typename R>
struct operation: operation_base<R>
{
    void start() &
    {
        operation_base<R>::enqueue();
    }
};

template <typename R>
struct operation_after: operation_base<R>
{
    std::chrono::milliseconds _dt;

    void start() &
    {
        operation_base<R>::enqueue(_dt);
    }
};

template <typename R>
struct operation_at: operation_base<R>
{
    time_point_t _dl;

    void start() &
    {
        operation_base<R>::enqueue(_dl);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    thread_pool* _pool;

    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation<R>{{{}, _pool, std::forward<R>(receiver)}};
    }
};

struct sender_after
{
    thread_pool* _pool;
    std::chrono::milliseconds _dt;

    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation_after<R>{{{}, _pool, std::forward<R>(receiver)}, _dt};
    }
};

struct sender_at
{
    thread_pool* _pool;
    time_point_t _dl;

    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation_at<R>{{{}, _pool, std::forward<R>(receiver)}, _dl};
    }
};

////////////////////////////////////////////////////////////////////////////////

struct scheduler
{
    thread_pool* _pool;

    constexpr bool operator == (scheduler const &) const noexcept = default;

    auto schedule() const noexcept
    {
        return sender{_pool};
    }

    template <typename R, typename P>
    auto schedule_after(std::chrono::duration<R, P> dt)
    {
        return sender_after{
            _pool,
            std::chrono::duration_cast<std::chrono::milliseconds>(dt)
        };
    }

    auto schedule_at(time_point_t dl)
    {
        return sender_at{_pool, dl};
    }

    template <typename S, typename I, typename F>
    friend auto tag_invoke(tag_t<execution::bulk>,
        scheduler const& self,
        S&& sender,
        I shape,
        F&& func) -> bulk_impl::sender<std::decay_t<S>, std::decay_t<I>, std::decay_t<F>>
    {
        return {self._pool, std::forward<S>(sender), shape, std::forward<F>(func)};
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct sender_traits_base
{
    using operation_t = T;
    using values_t = meta::list<signature<>>;
    using errors_t = meta::list<std::exception_ptr>;
};

template <typename R>
using sender_traits = sender_traits_base<operation<R>>;

template <typename R>
using sender_after_traits = sender_traits_base<operation_after<R>>;

template <typename R>
using sender_at_traits = sender_traits_base<operation_at<R>>;

}   // namespace simple_thread_pool_impl

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct sender_traits<simple_thread_pool_impl::sender, R>
    : simple_thread_pool_impl::sender_traits<R>
{};

template <typename R>
struct sender_traits<simple_thread_pool_impl::sender_after, R>
    : simple_thread_pool_impl::sender_after_traits<R>
{};

template <typename R>
struct sender_traits<simple_thread_pool_impl::sender_at, R>
    : simple_thread_pool_impl::sender_at_traits<R>
{};

}   // namespace execution
