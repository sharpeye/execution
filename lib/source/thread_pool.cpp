#include <execution/thread_pool.hpp>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

thread_pool::thread_pool(std::size_t worker_count)
    : thread_pool_impl {worker_count}
{
    thread_pool_impl::start();
}

thread_pool::~thread_pool()
{
    stop();
}

void thread_pool::schedule(task_base* task)
{
    _queue.enqueue(task);
}

void thread_pool::stop()
{
    if (_should_stop.test_and_set()) {
        return;
    }

    for (std::size_t n = thread_pool_impl::size(); n; --n) {
        _queue.enqueue(nullptr);
    }

    thread_pool_impl::join();
}

}   // namespace execution
