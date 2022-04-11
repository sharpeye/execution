#pragma once

#include "context.hpp"

#include <netinet/ip.h>

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

    union {
        sockaddr addr;
        sockaddr_in addr4;
        sockaddr_in6 addr6;
    } _peer;

    socklen_t _peer_len;

    template <typename U>
    operation(
            U&& receiver,
            context* ctx,
            int fd)
        : _receiver{std::forward<U>(receiver)}
        , _ctx{ctx}
        , _fd{fd}
        , _peer{}
        , _peer_len{sizeof(_peer)}
    {}

    void start() & noexcept
    {
        auto* sqe = _ctx->get_sqe();
        assert(sqe);

        io_uring_prep_accept(sqe, _fd, &_peer.addr, &_peer_len, 0);
        io_uring_sqe_set_data(sqe, this);

        _ctx->submit();
    }

    void completion(io_uring_cqe* cqe) noexcept override
    {
        if (auto ec = make_error_code(cqe->res)) {
            execution::set_error(std::move(_receiver), ec);
        } else {
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
    using values_t = meta::list<
        signature<int, sockaddr_in>,
        signature<int, sockaddr_in6>
    >;
    using errors_t = meta::list<std::error_code>;
};

}   // namespace execution

////////////////////////////////////////////////////////////////////////////////

namespace uring {

////////////////////////////////////////////////////////////////////////////////

constexpr auto accept = accept_impl::accept{};

}   // namespace uring
