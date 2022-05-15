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

namespace execution::simple_thread_pool_impl {

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

}   // namespace execution::simple_thread_pool_impl
