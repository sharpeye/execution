#pragma once

#include "sender_traits.hpp"
#include "stop_token.hpp"
#include "task_queue.hpp"
#include "thread_pool_bulk.hpp"

#include <functional>
#include <utility>

namespace execution {

namespace thread_pool_scheduler_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename R>
struct operation
    : task_base
{
    F _func;
    R _receiver;

    template <typename T, typename U>
    operation(T&& func, U&& receiver)
        : task_base {
            ._execute = static_cast<task_base::execute_t>(&operation::execute)
        }
        , _func {std::forward<T>(func)}
        , _receiver {std::forward<U>(receiver)}
    {}

    void start() &
    {
        std::invoke(_func, this);
    }

    void execute()
    {
        if (execution::get_stop_token(_receiver).stop_requested()) {
            execution::set_stopped(std::move(_receiver));
        } else {
            execution::set_value(std::move(_receiver));
        }
    }
};

template<typename F, typename R>
operation(F&&, R&&) -> operation<std::decay_t<F>, std::decay_t<R>>;

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct sender
{
    T* _pool;
    
    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation {
            [pool = _pool] (auto* task) { pool->schedule(task); },
            std::forward<R>(receiver)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename U>
struct sender_at
{
    T* _pool;
    U _deadline;

    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation {
            [pool = _pool, dl = _deadline] (auto* task) {
                pool->schedule_at(dl, task);
            },
            std::forward<R>(receiver)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename U>
struct sender_after
{
    T* _pool;
    U _duration;
    
    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation {
            [pool = _pool, dt = _duration] (auto* task) {
                pool->schedule_after(dt, task);
            },
            std::forward<R>(receiver)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct scheduler
{
    T* _pool;

    auto schedule() -> sender<T>
    {
        return {_pool};
    }

    template <typename U>
    auto schedule_at(U deadline) -> sender_at<T, U>
    {
        return {_pool, deadline};
    }

    template <typename U>
    auto schedule_after(U dt) -> sender_after<T, U>
    {
        return {_pool, dt};
    }

    bool operator == (scheduler const&) const noexcept = default; 

    template <typename S, typename I, typename F>
    friend auto tag_invoke(tag_t<execution::bulk>,
        scheduler<T> const& self,
        S&& sender,
        I shape,
        F&& func) -> thread_pool_bulk_impl::sender<
            T,
            std::decay_t<S>,
            std::decay_t<I>,
            std::decay_t<F>>
    {
        return {self._pool, std::forward<S>(sender), shape, std::forward<F>(func)};
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename R>
struct sender_traits_base
{
    using operation_t = decltype(std::declval<S>().connect(std::declval<R>()));
    using values_t = meta::list<signature<>>;
    using errors_t = meta::list<std::exception_ptr>;
};

template <typename T, typename R>
using sender_traits = sender_traits_base<sender<T>, R>;

template <typename T, typename U, typename R>
using sender_at_traits = sender_traits_base<sender_at<T, U>, R>;

template <typename T, typename U, typename R>
using sender_after_traits = sender_traits_base<sender_after<T, U>, R>;

}   // namespace thread_pool_scheduler_impl

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename R>
struct sender_traits<thread_pool_scheduler_impl::sender<T>, R>
    : thread_pool_scheduler_impl::sender_traits<T, R>
{
};

template <typename T, typename U, typename R>
struct sender_traits<thread_pool_scheduler_impl::sender_at<T, U>, R>
    : thread_pool_scheduler_impl::sender_at_traits<T, U, R>
{
};

template <typename T, typename U, typename R>
struct sender_traits<thread_pool_scheduler_impl::sender_after<T, U>, R>
    : thread_pool_scheduler_impl::sender_after_traits<T, U, R>
{
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
using thread_pool_scheduler = thread_pool_scheduler_impl::scheduler<T>;

}   // namespace execution
