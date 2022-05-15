#include <execution/simple_thread_pool.hpp>

#include <cassert>

namespace execution::simple_thread_pool_impl {

////////////////////////////////////////////////////////////////////////////////

thread_pool::thread_pool(std::size_t thread_count)
{
    _threads.reserve(thread_count);
    for (std::size_t i = 0; i != thread_count; ++i) {
        _threads.emplace_back([this] {
            worker();
        });
    }

    _threads.emplace_back([this] {
        timed_worker();
    });
}

thread_pool::~thread_pool()
{
    shutdown();
}

void thread_pool::shutdown()
{
    _should_stop.test_and_set();
    _cv.notify_all();
    _timed_cv.notify_all();

    for (auto& t: _threads) {
        t.join();
    }

    while (!_urgent_tasks.empty()) {
        _urgent_tasks.front()->execute();
        _urgent_tasks.pop();
    }

    while (!_tasks.empty()) {
        _tasks.front()->execute();
        _tasks.pop();
    }

    while (!_timed_queue.empty()) {
        _timed_queue.top().task->execute();
        _timed_queue.pop();
    }
}

void thread_pool::enqueue(task_base* t)
{
    std::unique_lock lock {_mtx};

    _tasks.push(t);
    _cv.notify_one();
}

void thread_pool::enqueue_urgent(task_base* t)
{
    std::unique_lock lock {_mtx};

    _urgent_tasks.push(t);
    _cv.notify_one();
}

void thread_pool::enqueue(time_point_t deadline, task_base* t)
{
    std::unique_lock lock {_timed_mtx};

    _timed_queue.push({ t, deadline });
    _timed_cv.notify_all();
}

task_base* thread_pool::dequeue()
{
    std::unique_lock lock {_mtx};
    _cv.wait(lock, [this] {
        return !_urgent_tasks.empty() || !_tasks.empty() || _should_stop.test();
    });

    if (_should_stop.test()) {
        return {};
    }

    auto& q = _urgent_tasks.empty()
        ? _tasks
        : _urgent_tasks;

    auto t = std::move(q.front());
    q.pop();
    return t;
}

void thread_pool::worker()
{
    for (;;) {
        task_base* task = dequeue();
        if (!task) {
            break;
        }

        task->execute();
    }
}

void thread_pool::timed_worker()
{
    for (;;) {
        std::unique_lock lock { _timed_mtx };

        const auto tp = _timed_queue.empty()
            ? time_point_t::max()
            : _timed_queue.top().deadline;

        _timed_cv.wait_until(lock, tp, [this, tp] {
            return _should_stop.test()
                || (!_timed_queue.empty() && _timed_queue.top().deadline < tp);
        });

        if (_should_stop.test()) {
            break;
        }

        auto now = clock_t::now();

        while (!_timed_queue.empty() && _timed_queue.top().deadline <= now) {
            enqueue_urgent(_timed_queue.top().task);
            _timed_queue.pop();
        }
    }
}

auto thread_pool::get_scheduler() noexcept -> scheduler
{
    return scheduler{this};
}

}   // namespace execution::simple_thread_pool_impl
