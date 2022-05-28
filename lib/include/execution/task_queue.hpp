#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

struct task_base
{
    using execute_t = void (task_base::*)();

    execute_t _execute = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

class task_queue
{
private:
    std::mutex _mtx;
    std::condition_variable _cv;
    std::queue<task_base*> _tasks;

public:
    void enqueue(task_base* task)
    {
        std::unique_lock lock {_mtx};

        _tasks.push(task);
        _cv.notify_one();
    }

    task_base* dequeue()
    {
        std::unique_lock lock {_mtx};
        _cv.wait(lock, [this] {
            return !_tasks.empty();
        });

        return try_dequeue_impl();
    }

    task_base* try_dequeue()
    {
        std::unique_lock lock {_mtx};

        return try_dequeue_impl();
    }

private:
    task_base* try_dequeue_impl()
    {
        if (_tasks.empty()) {
            return {};
        }

        task_base* t = _tasks.front();
        _tasks.pop();
        return t;
    }
};

////////////////////////////////////////////////////////////////////////////////

class priority_task_queue
{
    using queue_t = std::queue<task_base*>;

private:
    std::mutex _mtx;
    std::condition_variable _cv;

    queue_t _hi;
    queue_t _lo;

public:
    void enqueue_hi(task_base* task)
    {
        enqueue(_hi, task);
    }

    void enqueue_lo(task_base* task)
    {
        enqueue(_lo, task);
    }

    task_base* dequeue()
    {
        std::unique_lock lock {_mtx};

        _cv.wait(lock, [this] {
            return !_hi.empty() || !_lo.empty();
        });

        return try_dequeue_impl();
    }

    task_base* try_dequeue()
    {
        std::unique_lock lock {_mtx};

        return try_dequeue_impl();
    }

private:
    void enqueue(queue_t& q, task_base* task)
    {
        std::unique_lock lock {_mtx};

        q.push(task);
        _cv.notify_one();
    }

    task_base* try_dequeue_impl()
    {
        auto* q =
            !_hi.empty() ? &_hi :
            !_lo.empty() ? &_lo : nullptr;

        if (!q) {
            return {};
        }

        auto* t = q->front();
        q->pop();
        return t;
    }
};

}   // namespace execution
