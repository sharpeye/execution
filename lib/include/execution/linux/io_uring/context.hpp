#pragma once

#include <execution/meta.hpp>
#include <execution/sender_traits.hpp>
#include <execution/set_value.hpp>

#include <liburing.h>

#include <ctime>
#include <cassert>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include <iostream>

namespace execution::io_uring {

////////////////////////////////////////////////////////////////////////////////

struct operation_base
{
    using completion_t = void (operation_base::*)(int32_t res) noexcept;

    completion_t completion = nullptr;
};

using completion_t = operation_base::completion_t;

////////////////////////////////////////////////////////////////////////////////

class scheduler;

class context
    : operation_base
{
private:
    ::io_uring _ring;

    // TODO: lockfree queue
    std::mutex _mtx;
    std::vector<operation_base*> _operations;

    std::atomic_flag _should_stop;
    int _inflight = 0;
    int _eventfd = -1;

public:
    context();

    context(context const&) = delete;
    context(context &&) = delete;
    context& operator = (context const&) = delete;
    context& operator = (context &&) = delete;

    ~context();

    void run();
    void shutdown();

    scheduler get_scheduler() noexcept;

    void schedule(operation_base* op);

    void prepare_poll_add(operation_base* op, int fd, short poll_mask);
    void prepare_timeout(
        operation_base* op,
        timespec& ts,
        unsigned count,
        unsigned flags);

    void prepare_accept(operation_base* op, int fd); // TODO: addr

private:
    void kick_eventfd();
    void poll_eventfd();
    void read_eventfd();

    void eventfd_completion(int32_t res) noexcept;

    io_uring_cqe* wait_cqe();
    void process(io_uring_cqe* cqe);

    void process_operations();
    io_uring_sqe* grab_sqe(operation_base* op);
    void submit();
};

namespace scheduler_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation
    : operation_base
{
    R receiver;
    context* ctx;

    void start()
    {
        ctx->schedule(this);
    }

    void handle_schedule(int32_t res) noexcept
    {
        if (res == -ECANCELED) {
            execution::set_stopped(std::move(receiver));

            return;
        }

        execution::set_value(receiver);
    }
};

template <typename R>
struct operation_after
    : operation_base
{
    R receiver;
    context* ctx;
    timespec ts;
    unsigned flags;

    void start()
    {
        ctx->schedule(this);
    }

    void handle_schedule(int32_t res) noexcept
    {
        if (res == -ECANCELED) {
            execution::set_stopped(std::move(receiver));
            return;
        }

        assert(!res);

        completion = static_cast<completion_t>(&operation_after::handle_timeout);
        ctx->prepare_timeout(this, ts, 0, flags);
    }

    void handle_timeout(int32_t res) noexcept
    {
        if (res == -ECANCELED) {
            execution::set_stopped(std::move(receiver));
            return;
        }

        assert(res == -ETIME);

        execution::set_value(std::move(receiver));
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    template <typename R>
    using operation_t = operation<R>;
    using values_t = meta::list<signature<>>;
    using errors_t = meta::list<>;

    context* ctx;

    template <typename R>
    auto connect(R&& receiver)
    {
        using T = operation_t<std::decay_t<R>>;

        return T {
            {static_cast<completion_t>(&T::handle_schedule)},
            std::forward<R>(receiver),
            ctx
        };
    }
};

struct sender_after
{
    template <typename R>
    using operation_t = operation_after<R>;
    using values_t = meta::list<signature<>>;
    using errors_t = meta::list<>;

    context* ctx;
    timespec ts;
    unsigned flags;

    template <typename R>
    auto connect(R&& receiver)
    {
        using T = operation_t<std::decay_t<R>>;

        return T {
            {static_cast<completion_t>(&T::handle_schedule)},
            std::forward<R>(receiver),
            ctx,
            ts,
            flags
        };
    }
};

}   // namespace scheduler_impl

////////////////////////////////////////////////////////////////////////////////

class scheduler
{
private:
    context* _ctx;

public:
    explicit scheduler(context& ctx)
        : _ctx {&ctx}
    {}

    constexpr context& get_context() noexcept
    {
        return *_ctx;
    }

    constexpr auto schedule() const noexcept
    {
        return scheduler_impl::sender {_ctx};
    }

    constexpr auto schedule_after(std::chrono::milliseconds dt) const noexcept
    {
        timespec ts {
            .tv_sec  = dt.count() / 1000,
            .tv_nsec = (dt.count() % 1000) * 1'000'000
        };

        return scheduler_impl::sender_after {_ctx, ts, 0};
    }

    auto schedule_at(
        std::chrono::steady_clock::time_point deadline) const noexcept
    {
        auto now = std::chrono::steady_clock::now();

        timespec ts {};
        unsigned flags = IORING_TIMEOUT_ABS;

        if (now < deadline) {
            auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - now
            ).count();

            clock_gettime(CLOCK_MONOTONIC, &ts);
            ts.tv_sec  += ms / 1000;
            ts.tv_nsec += (ms % 1000) * 1'000'000;
        } else {
            flags = 0;
        }

        return scheduler_impl::sender_after {_ctx, ts, flags};
    }

    constexpr bool operator == (scheduler const& rhs) const = default;
};

}   // namespace execution::io_uring
