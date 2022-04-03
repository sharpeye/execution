#pragma once

#include <execution/sender_traits.hpp>

#include <liburing.h>

#include <iostream>

#include <span>
#include <string_view>
#include <thread>
#include <stop_token>
#include <system_error>

namespace uring {

////////////////////////////////////////////////////////////////////////////////

struct operation_base
{
    virtual ~operation_base() = default;
    virtual void completion(io_uring_cqe* cqe) noexcept = 0;
};

////////////////////////////////////////////////////////////////////////////////

inline std::error_code make_error_code(int ec) noexcept
{
    if (ec < 0) {
        return std::error_code{-ec, std::system_category()};
    }
    return {};
}

////////////////////////////////////////////////////////////////////////////////

class context
{
private:
    io_uring _ring = {};
    io_uring_params _params = {};

    std::stop_source _stop_source;
    std::thread _thread;

public:
    context() = default;

    context(context const&) = delete;
    context(context&&) = default;

    context& operator = (context const&) = delete;
    context& operator = (context&&) = default;

    ~context() noexcept
    {
        stop();
    }

    void start(unsigned entries)
    {
        auto ec = make_error_code(io_uring_queue_init_params(
            entries, &_ring, &_params));

        if (ec) {
            throw std::system_error(ec);
        }

        _thread = std::thread(&context::loop, this);
    }

    void stop() noexcept
    {
        if (!_thread.joinable()) {
            return;
        }

        _stop_source.request_stop();

        io_uring_prep_nop(get_sqe());
        submit();

        _thread.join();

        io_uring_queue_exit(&_ring);
    }

    io_uring_sqe* get_sqe() noexcept
    {
        return io_uring_get_sqe(&_ring);
    }

    std::error_code wait_cqe(io_uring_cqe** cqe_ptr) noexcept
    {
        return make_error_code(io_uring_wait_cqe(&_ring, cqe_ptr));
    }

    void cqe_seen(io_uring_cqe* cqe) noexcept
    {
        io_uring_cqe_seen(&_ring, cqe);
    }

    std::error_code submit() noexcept
    {
        return make_error_code(io_uring_submit(&_ring));
    }

    std::stop_token get_stop_token() noexcept
    {
        return _stop_source.get_token();
    }

    // TODO: cancellation (io_uring_prep_cancel)

private:
    void loop()
    {
        while (!_stop_source.stop_requested()) {
            io_uring_cqe* cqe = nullptr;

            if (wait_cqe(&cqe)) {
                // XXX
                continue;
            }

            auto* op = static_cast<operation_base*>(io_uring_cqe_get_data(cqe));
            if (!op) {
                continue;
            }

            op->completion(cqe);

            cqe_seen(cqe);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

namespace write_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation
    : operation_base
{
    R _receiver;
    context* _ctx;
    int _fd;
    iovec _iov[1];
    // io_uring_sqe* _sqe;

    template <typename U>
    operation(
            U&& receiver,
            context* ctx,
            int fd,
            std::span<char const> buffer)
        : _receiver{std::forward<U>(receiver)}
        , _ctx{ctx}
        , _fd{fd}
        , _iov{{const_cast<char*>(buffer.data()), buffer.size()}}
    {}
    
    void start() & noexcept
    {
        auto* sqe = _ctx->get_sqe();

        io_uring_prep_writev(sqe, _fd, _iov, 1, 0);
        io_uring_sqe_set_data(sqe, this);

        _ctx->submit();
    }

    void completion(io_uring_cqe* cqe) noexcept override
    {
        if (cqe->res < 0) {
            execution::set_error(std::move(_receiver), make_error_code(-cqe->res));
        } else {
            execution::set_value(std::move(_receiver), cqe->res);
        }
    };
};

struct sender
{
    context* _ctx;
    int _fd;
    std::span<char const> _buffer;

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

struct write
{
    sender operator () (context& ctx, int fd, std::string_view s) const
    {
        return {&ctx, fd, {s.data(), s.size()}};
    }
};

}   // namespace write_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto write = write_impl::write{};

}   // namespace uring

////////////////////////////////////////////////////////////////////////////////

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct sender_traits<uring::write_impl::sender, R>
{
    using operation_t = uring::write_impl::operation<R>;
    using values_t = meta::list<signature<int>>;
    using errors_t = meta::list<std::error_code>;
};

}   // namespace execution
