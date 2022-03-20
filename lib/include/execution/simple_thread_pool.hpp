#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

class simple_thread_pool
{
    struct task
    {
        void* data;
        void (*execute)(void*);
        void (*cleanup)(void*);
    };

    //using task_t = std::function<void ()>;

private:
    std::vector<std::thread> _threads;
    std::queue<task> _queue;
    std::mutex _mtx;
    std::condition_variable _cv;

public:
    explicit simple_thread_pool(std::size_t thread_count)
    {
        _threads.reserve(thread_count);
        for (std::size_t i = 0; i != thread_count; ++i) {
            _threads.emplace_back([this, i] {
                worker();
            });
        }
    }

    template <typename F>
    void enqueue(F&& fn)
    {
        enqueue({
            .data = new F{std::forward<F>(fn)},
            .execute = [](void* data){
                std::invoke(*static_cast<F*>(data));
            },
            .cleanup = [](void* data){
                delete static_cast<F*>(data);
            }
        });
    }

    void shutdown()
    {
        for (auto& t: _threads) {
            enqueue({});
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

private:
    void enqueue(task t)
    {
        std::unique_lock lock {_mtx};
        _queue.emplace(t);
        _cv.notify_one();
    }

    task dequeue()
    {
        std::unique_lock lock {_mtx};
        _cv.wait(lock, [this] {
            return !_queue.empty();
        });

        auto task = std::move(_queue.front());
        _queue.pop();
        return task;
    }

    void worker()
    {
        for (;;) {
            auto task = dequeue();
            if (!task.execute) {
                break;
            }

            execute(task);
        }
    }

    void execute(task t)
    {
        t.execute(t.data);
        t.cleanup(t.data);
    }
};

}   // namespace execution
