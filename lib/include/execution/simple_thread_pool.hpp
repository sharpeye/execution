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

    // struct scheduler
    // {};

private:
    std::vector<std::thread> _threads;
    std::queue<task> _queue;
    std::mutex _mtx;
    std::condition_variable _cv;

public:
    explicit simple_thread_pool(std::size_t thread_count);

    void enqueue(void (*fn)());

    template <typename F>
    void enqueue(F&& fn)
    {
        enqueue({
            .data = new F{std::forward<F>(fn)},
            .execute = [] (void* data) {
                auto* f = static_cast<F*>(data);
                std::invoke(*f);
                delete f;
            },
        });
    }

    template <typename F, typename T, typename ... Ts>
    void enqueue(F&& fn, T&& arg0, Ts&& ... args)
    {
        enqueue([
            fn = std::forward(fn),
            arg0 = std::forward<T>(arg0),
            ...args = std::forward<Ts>(args)
        ] () mutable {
            std::invoke(std::move(fn), std::move(arg0), std::move(args)...);
        });
    }

    void shutdown();

    // scheduler get_scheduler() noexcept
    // {
    //     return scheduler{*this};
    // }

private:
    void enqueue(task t);
    task dequeue();

    void worker();
    void execute(task t);
};

}   // namespace execution
