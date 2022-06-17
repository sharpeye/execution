#pragma once

#include "context.hpp"

#include <execution/get_scheduler.hpp>

#include <system_error>

namespace execution::io_uring {

namespace accept_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation_descr
{
    R receiver;
    context* ctx;
    int fd;
};

////////////////////////////////////////////////////////////////////////////////

template <typename R>
class operation
    : operation_base
{
    // TODO: cancellation

private:
    R _receiver;
    context* _ctx;
    int _fd;

public:
    explicit operation(operation_descr<R>&& descr)
        : operation_base {static_cast<completion_t>(&operation::handle_accept)}
        , _receiver {std::move(descr.receiver)}
        , _ctx {descr.ctx}
        , _fd {descr.fd}
    {
    }

    void start() & noexcept
    {
        _ctx->prepare_accept(this, _fd);
    }

    void handle_accept(int32_t res) noexcept
    {
        if (res == -ECANCELED) {
            execution::set_stopped(std::move(_receiver));
            return;
        }

        if (res < 0) {
            execution::set_error(
                std::move(_receiver),
                std::error_code {res, std::system_category()}
            );
            return;
        }

        // TODO: raii for `res` (descriptor)
        execution::set_value(std::move(_receiver), res);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    template <typename R>
    using operation_t = operation<R>;
    using values_t = meta::list<signature<int>>;
    using errors_t = meta::list<std::error_code>;

    context* ctx;
    int fd;

    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation_descr<std::decay_t<R>> {
            std::forward<R>(receiver),
            ctx,
            fd
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct accept
{
    accept_impl::sender operator () (context& ctx, int fd) const
    {
        return {&ctx, fd};
    }
};

}   // namespace accept_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto accept = accept_impl::accept{};

}   // namespace execution::io_uring
