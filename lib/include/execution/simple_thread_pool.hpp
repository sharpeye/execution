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

struct task
{
    void* obj;
    void (*execute)(void*);
    void (*discard)(void*);
};

struct timed_task: task
{
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
    std::queue<task> _tasks;
    std::queue<task> _urgent_tasks;

    std::mutex _timed_mtx;
    std::condition_variable _timed_cv;
    std::priority_queue<timed_task> _timed_queue;

public:
    explicit thread_pool(std::size_t thread_count);
    ~thread_pool();

    template <typename F, typename ... Ts>
    void enqueue(F&& fn, Ts&& ... args)
    {
        enqueue(create_task(
            std::forward<F>(fn),
            std::forward<Ts>(args)...
        ));
    }

    template <typename F, typename ... Ts>
    void enqueue(time_point_t deadline, F&& fn, Ts&& ... args)
    {
        enqueue(deadline, create_task(
            std::forward<F>(fn),
            std::forward<Ts>(args)...
        ));
    }

    template <typename D, typename F, typename ... Ts>
    void enqueue(D dt, F&& fn, Ts&& ... args)
    {
        const auto deadline = clock_t::now() + dt;

        enqueue(deadline, create_task(
            std::forward<F>(fn),
            std::forward<Ts>(args)...
        ));
    }

    void shutdown();

    scheduler get_scheduler() noexcept;

private:
    template <typename F>
        requires std::is_convertible_v<F, void (*)()>
    task create_task(F&& fn)
    {
        static_assert(sizeof(void*) == sizeof(void (*)()));

        auto p = static_cast<void (*)()>(std::forward<F>(fn));

        return { .obj = reinterpret_cast<void*>(p) };
    }

    template <typename F>
    task create_task(F&& fn)
    {
        using T = std::decay_t<F>;

        return {
            .obj = new T{std::forward<F>(fn)},
            .execute = [] (void* obj) {
                auto* f = static_cast<T*>(obj);
                std::invoke(std::move(*f));
                delete f;
            },
            .discard = [] (void* obj) {
                auto* f = static_cast<T*>(obj);
                delete f;
            }
        };
    }

    template <typename F, typename T, typename ... Ts>
    void create_task(F&& fn, T&& arg0, Ts&& ... args)
    {
        return create_task([
            fn = std::forward<F>(fn),
            arg0 = std::forward<T>(arg0),
            ...args = std::forward<Ts>(args)
        ] () mutable {
            std::invoke(std::move(fn), std::move(arg0), std::move(args)...);
        });
    }

private:
    void enqueue(task t);
    void enqueue_urgent(task t);
    void enqueue(time_point_t deadline, task t);

    task dequeue();

    void worker();
    void timed_worker();
    void execute(task t);
    void discard(task t);
};

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation_base
{
    thread_pool* _pool;
    R _receiver;

    auto create_task()
    {
        return [this] () mutable {
            if (execution::get_stop_token(_receiver).stop_requested()) {
                execution::set_stopped(std::move(_receiver));
            } else {
                execution::set_value(std::move(_receiver));
            }
        };
    }

    void enqueue()
    {
        _pool->enqueue(create_task());
    }

    void enqueue(std::chrono::milliseconds dt)
    {
        _pool->enqueue(dt, create_task());
    }

    void enqueue(time_point_t dl)
    {
        _pool->enqueue(dl, create_task());
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
        return operation<R>{{_pool, std::forward<R>(receiver)}};
    }
};

struct sender_after
{
    thread_pool* _pool;
    std::chrono::milliseconds _dt;

    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation_after<R>{{_pool, std::forward<R>(receiver)}, _dt};
    }
};

struct sender_at
{
    thread_pool* _pool;
    time_point_t _dl;

    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation_at<R>{{_pool, std::forward<R>(receiver)}, _dl};
    }
};

////////////////////////////////////////////////////////////////////////////////

struct scheduler
{
    thread_pool* _pool;

    auto schedule() const noexcept
    {
        return sender{_pool};
    }

    auto schedule_after(std::chrono::milliseconds dt)
    {
        return sender_after{_pool, dt};
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
