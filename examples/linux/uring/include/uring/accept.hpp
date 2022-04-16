#pragma once

#include "context.hpp"

#include <netinet/ip.h>

#include <cassert>
#include <optional>

namespace uring {
namespace accept_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation_descr
{
    R _receiver;
    context* _ctx;
    int _fd;
};

template <typename R>
struct operation
    : operation_base
{
    struct cancel_callback
    {
        operation<R>* _this;

        void operator () () noexcept
        {
            _this->cancel();
        }
    };

    R _receiver;
    context* _ctx;
    int _fd;

    union {
        sockaddr addr;
        sockaddr_in addr4;
        sockaddr_in6 addr6;
    } _peer;

    socklen_t _peer_len;
    std::optional<std::stop_callback<cancel_callback>> _stop_callback;

    explicit operation(operation_descr<R>&& descr)
        : _receiver{std::move(descr._receiver)}
        , _ctx{descr._ctx}
        , _fd{descr._fd}
        , _peer{}
        , _peer_len{sizeof(_peer)}
    {}

    void start() & noexcept
    {
        auto* sqe = _ctx->get_sqe();
        assert(sqe);

        io_uring_prep_accept(sqe, _fd, &_peer.addr, &_peer_len, 0);
        io_uring_sqe_set_data(sqe, this);

        _stop_callback.emplace(
            execution::get_stop_token(_receiver),
            cancel_callback{this});

        _ctx->submit();
    }

    void cancel()
    {
        auto* sqe = _ctx->get_sqe();
        assert(sqe);

        io_uring_prep_cancel(sqe, this, 0);

        _ctx->submit();
    }

    void completion(io_uring_cqe* cqe) noexcept override
    {
        auto ec = make_error_code(cqe->res);

        if (ec == std::errc::operation_canceled) {
            execution::set_stopped(std::move(_receiver));
            return;
        }

        if (ec) {
            execution::set_error(std::move(_receiver), ec);
            return;
        }

        switch (_peer.addr.sa_family) {
            case AF_INET:
                execution::set_value(std::move(_receiver), cqe->res, _peer.addr4);
                break;
            case AF_INET6:
                execution::set_value(std::move(_receiver), cqe->res, _peer.addr6);
                break;
            default:
                execution::set_error(
                    std::move(_receiver),
                    std::make_error_code(std::errc::address_family_not_supported)
                );
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct sender
{
    template <typename R>
    using operation_t = operation<R>;
    using values_t = execution::meta::list<
        execution::signature<int, sockaddr_in>,
        execution::signature<int, sockaddr_in6>
    >;
    using errors_t = execution::meta::list<std::error_code>;

    context* _ctx;
    int _fd;

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation_descr<std::decay_t<R>>{
            std::forward<R>(receiver),
            _ctx,
            _fd
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation_descr<std::decay_t<R>>{
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

}   // namespace accept_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto accept = accept_impl::accept{};

}   // namespace uring
