#pragma once

#include "sender_traits.hpp"
#include "stop_token.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <queue>
#include <thread>
#include <vector>

namespace execution {
namespace simple_thread_pool_impl {

using clock_t = std::chrono::high_resolution_clock;
using time_point_t = clock_t::time_point;

struct scheduler;

////////////////////////////////////////////////////////////////////////////////

struct task_base
{
    using execute_t = void (task_base::*)();

    execute_t _execute;

    void execute()
    {
        std::invoke(_execute, this);
    }
};

struct timed_task
{
    task_base* task;
    time_point_t deadline;
};

inline bool operator < (timed_task lhs, timed_task rhs)
{
    return lhs.deadline > rhs.deadline;
}

////////////////////////////////////////////////////////////////////////////////

class thread_pool
{
private:
    std::atomic_flag _should_stop = false;
    std::vector<std::thread> _threads;

    std::mutex _mtx;
    std::condition_variable _cv;
    std::queue<task_base*> _tasks;
    std::queue<task_base*> _urgent_tasks;

    std::mutex _timed_mtx;
    std::condition_variable _timed_cv;
    std::priority_queue<timed_task> _timed_queue;

public:
    explicit thread_pool(std::size_t thread_count);
    ~thread_pool();

    void shutdown();
    scheduler get_scheduler() noexcept;

    void enqueue(task_base* t);
    void enqueue(time_point_t deadline, task_base* t);
    template <typename D>
    void enqueue(D dt, task_base* t)
    {
        enqueue(clock_t::now() + dt, t);
    }

private:
    void enqueue_urgent(task_base* t);
    task_base* dequeue();

    void worker();
    void timed_worker();
};

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

using simple_thread_pool = simple_thread_pool_impl::thread_pool;

}   // namespace execution
