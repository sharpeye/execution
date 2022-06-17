#pragma once

#include "context.hpp"

#include <byte>
#include <span>

namespace execution::io_uring {
namespace write_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation
    : operation_base
{
    R receiver;
    context* ctx;
    int fd;
    iovec iov;

    void start() noexcept
    {
        // ...
    }

    void handle_write(int32_t res) noexcept
    {
        /*
        if (res == -ECANCELED) {
            execution::set_stopped(std::move(receiver));
            return;
        }

        if (res < 0) {
            set_error(std::move(receiver), res);
        }

        execution::set_value(std::move(receiver));*/
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    template <typename R>
    using operation_t = operation<R>;
    using values_t = meta::list<signature<>>;
    using errors_t = meta::list<std::error_code>;

    context* ctx;
    int fd;
    std::span<std::byte const> buffer;

    template <typename R>
    auto connect(R&& receiver) const
    {
        using T = operation_t<std::decay_t<R>>;

        return T {
            {static_cast<completion_t>(&T::handle_write)},
            std::forward<R>(receiver),
            ctx,
            fd,
            buffer
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct write
{
    sender operator () (context& ctx, int fd, std::span<std::byte const> buf) const
    {
        return {&ctx, fd, buf};
    }
};

struct write_some
{
    sender_some operator () (context& ctx, int fd, std::span<std::byte const> buf) const
    {
        return {&ctx, fd, buf};
    }
};

}   // namespace write_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto write = write_impl::write{};

}   // namespace execution::io_uring
