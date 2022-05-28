#pragma once

#include "task_queue.hpp"
#include "thread_pool_impl.hpp"
#include "thread_pool_scheduler.hpp"

#include <atomic>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

class thread_pool
    : thread_pool_impl<thread_pool>
{
    friend thread_pool_impl;

private:
    task_queue _queue;
    std::atomic_flag _should_stop = {};

public:
    explicit thread_pool(std::size_t worker_count);
    ~thread_pool();

    void stop();

    void schedule(task_base* task);

    thread_pool_scheduler<thread_pool> get_scheduler()
    {
        return {this};
    }

private:
    task_queue& get_queue()
    {
        return _queue;
    }
};

}   // namespace execution
