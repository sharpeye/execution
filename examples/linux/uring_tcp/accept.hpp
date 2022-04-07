#pragma once

#include "context.hpp"

#include <cassert>

namespace uring::accept_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation
    : operation_base
{
    R _receiver;
    context* _ctx;
    int _fd;
    sockaddr_in _peer;
    // io_uring_sqe* _sqe; // for cancellation

    template <typename U>
    operation(
            U&& receiver,
            context* ctx,
            int fd)
        : _receiver{std::forward<U>(receiver)}
        , _ctx{ctx}
        , _fd{fd}
        , _peer{}
    {}

    void start() & noexcept
    {
        auto* sqe = _ctx->get_sqe();
        assert(sqe);

        io_uring_prep_accept(sqe, _fd, &_peer, sizeof(_peer), 0);
        io_uring_sqe_set_data(sqe, this);

        _ctx->submit();
    }

    void completion(io_uring_cqe* cqe) noexcept override
    {
        if (cqe->res < 0) {
            execution::set_error(std::move(_receiver), make_error_code(-cqe->res));
        } else {
            execution::set_value(std::move(_receiver), cqe->res, _peer);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    context* _ctx;
    int _fd;

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<R>{
            std::forward<R>(receiver),
            _ctx,
            _fd
        };
    }

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

struct accept
{
    sender operator () (context& ctx, int fd) const
    {
        return {&ctx, fd};
    }
};

}   // namespace uring::accept_impl

////////////////////////////////////////////////////////////////////////////////

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct sender_traits<uring::accept_impl::sender, R>
{
    using operation_t = uring::accept_impl::operation<R>;
    using values_t = meta::list<signature<int, sockaddr_in>>;
    using errors_t = meta::list<std::error_code>;
};

}   // namespace execution

////////////////////////////////////////////////////////////////////////////////

namespace uring {

////////////////////////////////////////////////////////////////////////////////

constexpr auto accept = accept_impl::accept{};

}   // namespace uring
