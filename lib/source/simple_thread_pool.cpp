#include <execution/simple_thread_pool.hpp>

#include <cassert>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

simple_thread_pool::simple_thread_pool(std::size_t thread_count)
{
    _threads.reserve(thread_count);
    for (std::size_t i = 0; i != thread_count; ++i) {
        _threads.emplace_back([this] {
            worker();
        });
    }
}

void simple_thread_pool::shutdown()
{
    for (auto& t: _threads) {
        enqueue(task{});
    }

    for (auto& t: _threads) {
        t.join();
    }

    // drain

    while (!_queue.empty()) {
        execute(_queue.front());
        _queue.pop();
    }
}

void simple_thread_pool::enqueue(void (*fn)())
{
    assert(fn);

    static_assert(sizeof(fn) == sizeof(void*));
    enqueue({
        .data = reinterpret_cast<void*>(fn),
        .execute = [] (void* data) {
            std::invoke(reinterpret_cast<void(*)()>(data));
        },
    });
}

void simple_thread_pool::enqueue(task t)
{
    std::unique_lock lock {_mtx};
    _queue.emplace(t);
    _cv.notify_one();
}

auto simple_thread_pool::dequeue() -> task
{
    std::unique_lock lock {_mtx};
    _cv.wait(lock, [this] {
        return !_queue.empty();
    });

    auto task = std::move(_queue.front());
    _queue.pop();
    return task;
}

void simple_thread_pool::worker()
{
    for (;;) {
        auto task = dequeue();
        if (!task.execute) {
            break;
        }

        execute(task);
    }
}

void simple_thread_pool::execute(task t)
{
    assert(t.execute);
    assert(t.data);

    t.execute(t.data);
}

}   // namespace execution
