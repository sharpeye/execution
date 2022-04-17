#pragma once

#include "context.hpp"

#include <execution/stop_token.hpp>

#include <cassert>
#include <span>

namespace uring {
namespace write_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation
    : operation_impl<operation<R>>
{
    R _receiver;
    context* _ctx;
    int _fd;
    iovec _iov;

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

    void completion(io_uring_cqe* cqe) noexcept
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
    template <typename R>
    using operation_t = operation<R>;
    using values_t = execution::meta::list<execution::signature<>>;
    using errors_t = execution::meta::list<std::error_code>;

    context* _ctx;
    int _fd;
    std::span<std::byte const> _buffer;

    template <typename R>
    auto connect(R&& receiver) const
    {
        return operation<R>{
            std::forward<R>(receiver),
            _ctx,
            _fd,
            _buffer
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

}   // namespace write_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto write = write_impl::write{};

}   // namespace uring
