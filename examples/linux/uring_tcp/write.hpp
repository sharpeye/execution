#pragma once

#include "context.hpp"

#include <execution/stop_token.hpp>

#include <cassert>
#include <span>

namespace uring::write_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation
    : operation_base
{
    R _receiver;
    context* _ctx;
    int _fd;
    iovec _iov;
    // io_uring_sqe* _sqe; // for cancellation

    template <typename U>
    operation(
            U&& receiver,
            context* ctx,
            int fd,
            std::span<std::byte const> buffer)
        : _receiver{std::forward<U>(receiver)}
        , _ctx{ctx}
        , _fd{fd}
        , _iov{const_cast<std::byte*>(buffer.data()), buffer.size()}
    {}

    void start() & noexcept
    {
        submit();
    }

    void completion(io_uring_cqe* cqe) noexcept override
    {
        using namespace ::execution;

        if (get_stop_token(_receiver).stop_requested()) {
            set_stopped(std::move(_receiver));
            return;
        }

        if (auto ec = make_error_code(cqe->res)) {
            set_error(std::move(_receiver), ec);
            return;
        }

        _iov.iov_len -= cqe->res;

        if (!_iov.iov_len) {
            set_value(std::move(_receiver));
            return;
        }

        _iov.iov_base = static_cast<char*>(_iov.iov_base) + cqe->res;

        submit();
    }

    void submit() noexcept
    {
        auto* sqe = _ctx->get_sqe();
        assert(sqe);

        io_uring_prep_writev(sqe, _fd, &_iov, 1, 0);
        io_uring_sqe_set_data(sqe, this);

        _ctx->submit();
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    context* _ctx;
    int _fd;
    std::span<std::byte const> _buffer;

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<R>{
            std::forward<R>(receiver),
            _ctx,
            _fd,
            _buffer
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<R>{
            std::forward<R>(receiver),
            _ctx,
            _fd,
            std::move(_buffer)
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

}   // namespace uring::write_impl

////////////////////////////////////////////////////////////////////////////////

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct sender_traits<uring::write_impl::sender, R>
{
    using operation_t = uring::write_impl::operation<R>;
    using values_t = meta::list<signature<>>;
    using errors_t = meta::list<std::error_code>;
};

}   // namespace execution

////////////////////////////////////////////////////////////////////////////////

namespace uring {

////////////////////////////////////////////////////////////////////////////////

constexpr auto write = write_impl::write{};

}   // namespace uring
