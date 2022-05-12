#pragma once

#include "context.hpp"

#include <cassert>

namespace uring {
namespace close_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation
    : operation_impl<operation<R>>
{
    R _receiver;
    context* _ctx;
    int _fd;

    template <typename U>
    operation(
            U&& receiver,
            context* ctx,
            int fd)
        : _receiver{std::forward<U>(receiver)}
        , _ctx{ctx}
        , _fd{fd}
    {}

    void start() & noexcept
    {
        auto ec = _ctx->submit_op([this] (io_uring_sqe* sqe) {
            io_uring_prep_close(sqe, _fd);
            io_uring_sqe_set_data(sqe, this);
        });

        if (ec) {
            execution::set_error(std::move(_receiver), ec);
        }
    }

    void completion(io_uring_cqe*) noexcept
    {
        execution::set_value(std::move(_receiver));
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    template <typename R>
    using operation_t = operation<R>;
    using values_t = execution::meta::list<execution::signature<>>;
    using errors_t = execution::meta::list<>;

    context* _ctx;
    int _fd;

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<R>{
            std::forward<R>(receiver),
            _ctx,
            _fd
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct close
{
    sender operator () (context& ctx, int fd) const
    {
        return {&ctx, fd};
    }
};

}   // namespace close_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto close = close_impl::close{};

}   // namespace uring
