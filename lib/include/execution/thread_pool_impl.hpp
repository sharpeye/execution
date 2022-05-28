#pragma once

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename Derived>
class thread_pool_impl
{
private:
    std::vector<std::thread> _threads;

public:
    explicit thread_pool_impl(std::size_t worker_count)
    {
        _threads.reserve(worker_count);
    }

    void start()
    {
        for (std::size_t n = _threads.capacity(); n; --n) {
            _threads.emplace_back([this] {
                worker();
            });
        }
    }

    std::size_t size() const
    {
        return _threads.size();
    }

    void join()
    {
        for (auto& t: _threads) {
            t.join();
        }
    }

private:
    void worker()
    {
        auto& queue = static_cast<Derived*>(this)->get_queue();

        for (;;) {
            auto* task = queue.dequeue();
            if (!task) {
                break;
            }

            std::invoke(task->_execute, task);
        }
    }
};

}   // namespace execution
