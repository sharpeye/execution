#pragma once

#include "task_queue.hpp"
#include "thread_pool_impl.hpp"
#include "thread_pool_scheduler.hpp"

#include <atomic>
#include <chrono>
#include <utility>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

class timed_thread_pool
    : thread_pool_impl<timed_thread_pool>
{
    friend thread_pool_impl;

public:
    using clock_t = std::chrono::steady_clock;
    using time_point_t = clock_t::time_point;

private:
    using scheduled_task_t = std::pair<task_base*, time_point_t>;

    struct compare
    {
        constexpr bool operator () (
            scheduled_task_t lhs,
            scheduled_task_t rhs) const
        {
            return lhs.second > rhs.second;
        }
    };

    using scheduled_tasks_t = std::priority_queue<
        scheduled_task_t,
        std::vector<scheduled_task_t>,
        compare
    >;

private:
    priority_task_queue _queue;

    std::mutex _mtx;
    std::condition_variable _cv;
    scheduled_tasks_t _scheduled_tasks;
    std::thread _dispatcher;

    std::atomic_flag _should_stop = {};

public:
    explicit timed_thread_pool(std::size_t worker_count);
    ~timed_thread_pool();

    void schedule(task_base* task);
    void schedule_at(time_point_t deadline, task_base* task);

    template <typename D>
    void schedule_after(D dt, task_base* task)
    {
        schedule_at(clock_t::now() + dt, task);
    }

    void stop();

    thread_pool_scheduler<timed_thread_pool> get_scheduler()
    {
        return {this};
    }

private:
    bool has_expired_task(time_point_t deadline) const;
    void dispatcher();

    priority_task_queue& get_queue()
    {
        return _queue;
    }
};

}   // namespace execution
