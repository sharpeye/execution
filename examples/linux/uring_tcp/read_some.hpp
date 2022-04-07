#pragma once

#include "context.hpp"

#include <cassert>
#include <span>

namespace uring::read_some_impl {

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
            std::span<std::byte> buffer)
        : _receiver{std::forward<U>(receiver)}
        , _ctx{ctx}
        , _fd{fd}
        , _iov{const_cast<std::byte*>(buffer.data()), buffer.size()}
    {}

    void start() & noexcept
    {
        auto* sqe = _ctx->get_sqe();
        assert(sqe);

        io_uring_prep_readv(sqe, _fd, &_iov, 1, 0);
        io_uring_sqe_set_data(sqe, this);

        _ctx->submit();
    }

    void completion(io_uring_cqe* cqe) noexcept override
    {
        using namespace ::execution;

        if (get_stop_token(_receiver).stop_requested()) {
            set_stopped(std::move(_receiver));
            return;
        }

        if (cqe->res < 0) {
            set_error(std::move(_receiver), make_error_code(-cqe->res));
            return;
        }

        set_value(
            std::move(_receiver),
            std::span<std::byte>{
                reinterpret_cast<std::byte*>(_iov.iov_base),
                cqe->res
            }
        );
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    context* _ctx;
    int _fd;
    std::span<std::byte> _buffer;

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

struct read_some
{
    sender operator () (context& ctx, int fd, std::string_view s) const
    {
        return {&ctx, fd, {
            reinterpret_cast<std::byte const*>(s.data()), s.size()
        }};
    }
};

}   // namespace uring::read_some_impl

////////////////////////////////////////////////////////////////////////////////

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct sender_traits<uring::read_some_impl::sender, R>
{
    using operation_t = uring::read_some_impl::operation<R>;
    using values_t = meta::list<signature<std::span<std::byte>>>;
    using errors_t = meta::list<std::error_code>;
};

}   // namespace execution

////////////////////////////////////////////////////////////////////////////////

namespace uring {

////////////////////////////////////////////////////////////////////////////////

constexpr auto read_some = read_some_impl::read_some{};

}   // namespace uring
