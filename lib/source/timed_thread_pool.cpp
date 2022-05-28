#include <execution/timed_thread_pool.hpp>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

timed_thread_pool::timed_thread_pool(std::size_t worker_count)
    : thread_pool_impl {worker_count}
    , _dispatcher {&timed_thread_pool::dispatcher, this}
{
    thread_pool_impl::start();
}

timed_thread_pool::~timed_thread_pool()
{
    stop();
}

void timed_thread_pool::schedule(task_base* task)
{
    _queue.enqueue_lo(task);
}

void timed_thread_pool::schedule_at(time_point_t deadline, task_base* task)
{
    std::unique_lock lock {_mtx};

    _scheduled_tasks.push({ task, deadline });
    _cv.notify_one();
}

void timed_thread_pool::stop()
{
    if (_should_stop.test_and_set()) {
        return;
    }

    schedule_at({}, nullptr);
    _dispatcher.join();

    for (std::size_t n = thread_pool_impl::size(); n; --n) {
        _queue.enqueue_lo(nullptr);
    }

    thread_pool_impl::join();
}

bool timed_thread_pool::has_expired_task(time_point_t deadline) const
{
    return !_scheduled_tasks.empty()
        && _scheduled_tasks.top().second < deadline;
}

void timed_thread_pool::dispatcher()
{
    while (!_should_stop.test()) {
        std::unique_lock lock {_mtx};

        auto const deadline = _scheduled_tasks.empty()
            ? time_point_t::max()
            : _scheduled_tasks.top().second;

        _cv.wait_until(lock, deadline, [this, deadline] {
            return has_expired_task(deadline);
        });

        auto const now = ++clock_t::now();

        while (has_expired_task(now)) {
            task_base* task = _scheduled_tasks.top().first;
            _scheduled_tasks.pop();
            if (!task) {
                continue;
            }
            _queue.enqueue_hi(task);
        }
    }

    while (!_scheduled_tasks.empty()) {
        task_base* task = _scheduled_tasks.top().first;
        _scheduled_tasks.pop();
        _queue.enqueue_hi(task);
    }
}

}   // namespace execution
