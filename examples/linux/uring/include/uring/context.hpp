#pragma once

#include <execution/sender_traits.hpp>

#include <liburing.h>

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
    return ec < 0
        ? std::error_code{-ec, std::system_category()}
        : std::error_code{};
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
            if (op) {
                op->completion(cqe);
            }

            cqe_seen(cqe);
        }
    }
};

}   // namespace uring
