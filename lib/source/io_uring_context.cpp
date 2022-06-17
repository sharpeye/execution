#include <execution/linux/io_uring/context.hpp>

#include <cassert>
#include <system_error>

#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace execution::io_uring {

namespace {

////////////////////////////////////////////////////////////////////////////////

std::error_code make_error_code(int ec) noexcept
{
    return ec < 0
        ? std::error_code {-ec, std::system_category()}
        : std::error_code {};
}

}   // namespace

context::context()
    : operation_base {
        .completion = static_cast<operation_base::completion_t>(
            &context::eventfd_completion)
    }
{
    _eventfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (_eventfd < 0) {
        int ec = errno;
        throw std::system_error {make_error_code(ec), "eventfd"};
    }

    auto ec = make_error_code(io_uring_queue_init(1024, &_ring, 0));
    if (ec) {
        throw std::system_error {ec, "io_uring_queue_init_params"};
    }
}

context::~context()
{
    io_uring_queue_exit(&_ring);

    ::close(_eventfd);
}

void context::poll_eventfd()
{
    prepare_poll_add(this, _eventfd, POLLIN);
}

void context::kick_eventfd()
{
    eventfd_t val {1};
    int r = ::write(_eventfd, &val, sizeof(val));
    if (r < 0) {
        int ec = errno;
        throw std::system_error {make_error_code(ec), "write"};
    }
    assert(r == sizeof(val));
}

void context::read_eventfd()
{
    eventfd_t val {};
    int r = ::read(_eventfd, &val, sizeof(val));
    if (r < 0) {
        int ec = errno;
        throw std::system_error {make_error_code(ec), "read"};
    }
    assert(r == sizeof(val));
}

void context::eventfd_completion(int res) noexcept
{
    assert(res >= 0);

    if (res < 0) {
        return;
    }

    read_eventfd();

    process_operations();

    if (!_should_stop.test()) {
        poll_eventfd();
    }
}

void context::process(io_uring_cqe* cqe)
{
    auto* op = static_cast<operation_base*>(io_uring_cqe_get_data(cqe));

    if (op) {
        assert(op->completion);
        std::invoke(op->completion, op, cqe->res);
    }
}

io_uring_cqe* context::wait_cqe()
{
    io_uring_cqe* cqe;
    int ec = io_uring_wait_cqe(&_ring, &cqe);
    if (ec < 0) {
        throw std::system_error {make_error_code(ec), "io_uring_wait_cqe"};
    }

    return cqe;
}

void context::run()
{
    poll_eventfd();

    while (_inflight) {
        ::io_uring_submit_and_wait(&_ring, 1);

        unsigned head;
        unsigned count = 0;
        io_uring_cqe* cqe;
  
        io_uring_for_each_cqe(&_ring, head, cqe) {
            ++count;
            process(cqe);
        }

        io_uring_cq_advance(&_ring, count);
        _inflight -= count;
    }

    process_operations();
}

void context::shutdown()
{
    if (!_should_stop.test_and_set()) {
        kick_eventfd();
    }
}

void context::process_operations()
{
    int32_t const res = _should_stop.test()
        ? -ECANCELED
        : 0;

    std::vector<operation_base*> ops;

    {
        std::unique_lock lock {_mtx};
        ops.swap(_operations);
    }

    for (auto* op: ops) {
        assert(op->completion);
        std::invoke(op->completion, op, res);
    }
}

void context::schedule(operation_base* op)
{
    if (_should_stop.test()) {
        assert(op->completion);
        std::invoke(op->completion, op, -ECANCELED);
        return;
    }

    {
        std::unique_lock lock {_mtx};
        _operations.push_back(op);
    }

    kick_eventfd();
}

scheduler context::get_scheduler() noexcept
{
    return scheduler {*this};
}

io_uring_sqe* context::grab_sqe(operation_base* op)
{
    auto* sqe = io_uring_get_sqe(&_ring);
    assert(sqe);

    if (!sqe) {
        std::invoke(op->completion, op, -EBUSY);
    }

    _inflight += !!sqe;

    return sqe;
}

void context::submit()
{
    int ec = io_uring_submit(&_ring);
    if (ec < 0) {
        throw std::system_error {make_error_code(ec), "io_uring_submit"};
    }
}

void context::prepare_poll_add(operation_base* op, int fd, short poll_mask)
{
    auto* sqe = grab_sqe(op);
    if (!sqe) {
        return;
    }

    io_uring_prep_poll_add(sqe, fd, poll_mask);
    io_uring_sqe_set_data(sqe, op);
}

void context::prepare_timeout(
    operation_base* op,
    timespec& ts,
    unsigned count,
    unsigned flags)
{
    static_assert(sizeof(__kernel_timespec) == sizeof(timespec));

    auto* sqe = grab_sqe(op);
    if (!sqe) {
        return;
    }

    io_uring_prep_timeout(sqe, reinterpret_cast<__kernel_timespec*>(&ts), count, flags);
    io_uring_sqe_set_data(sqe, op);
}

void context::prepare_accept(operation_base* op, int fd)
{
    auto* sqe = grab_sqe(op);
    if (!sqe) {
        return;
    }

    io_uring_prep_accept(sqe, fd, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, op);
}

}   // namespace execution::io_uring
