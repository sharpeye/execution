#include "system.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <cassert>

#include <iostream>

namespace actors {
namespace {

////////////////////////////////////////////////////////////////////////////////

class event_queue
{
private:
    std::queue<event_ptr_t> _events;
    std::mutex _mtx;
    std::condition_variable _has_event;

public:
    void push(event_ptr_t ev)
    {
        std::unique_lock lock {_mtx};

        _events.push(std::move(ev));
        _has_event.notify_one();
    }

    event_ptr_t pop()
    {
        std::unique_lock lock {_mtx};

        _has_event.wait(lock, [this] {
            return !_events.empty();
        });

        event_ptr_t event = std::move(_events.front());
        _events.pop();

        return event;
    }

    event_ptr_t try_pop()
    {
        std::unique_lock lock {_mtx};

        if (_events.empty()) {
            return {};
        }

        event_ptr_t event = std::move(_events.front());
        _events.pop();

        return event;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct register_actor
    : event
{
    actor_ptr_t _newbie;

    register_actor(actor_ptr_t newbie, actor_id id)
        : event(event_kind::register_actor, {}, id)
        , _newbie {std::move(newbie)}
    {}
};

////////////////////////////////////////////////////////////////////////////////

class system_impl final
    : public system
{
private:
    std::thread _worker;
    std::atomic_flag _should_stop;

    event_queue _queue;

    std::atomic<int> _ids;

    std::unordered_map<actor_id, actor_ptr_t> _alive;

public:
    ~system_impl()
    {
        stop();
    }

    void start() override
    {
        _worker = std::thread(&system_impl::worker, this);
    }

    void stop() override
    {
        if (_should_stop.test_and_set() && _worker.joinable()) {
            _queue.push(std::make_unique<event>());
            _worker.join();
        }
    }

    void send(event_ptr_t ev) override
    {
        _queue.push(std::move(ev));
    }

    actor_id spawn(actor_ptr_t newbie) override
    {
        auto id = static_cast<actor_id>(_ids.fetch_add(1));

        system::send<register_actor>(std::move(newbie), id);

        return id;
    }

private:

    void worker()
    {
        while (!_should_stop.test()) {
            process(_queue.pop());
        }

        while (auto ev = _queue.try_pop()) {
            process(std::move(ev));
        }
    }

    void process(event_ptr_t ev)
    {
        if (ev->_kind == event_kind::null) {
            return;
        }

        if (ev->_kind == event_kind::register_actor) {
            auto newbe = std::move(static_cast<register_actor&>(*ev)._newbie);

            auto [it, ok] = _alive.emplace(ev->_target, std::move(newbe));
            assert(ok);

            context ctx {ev->_target, *this};

            event bootstrap {event_kind::bootstrap, {}, ev->_target};
            it->second->process(ctx, bootstrap);

            return;
        }

        auto it = _alive.find(ev->_target);
        if (it == _alive.end()) {
            return;
        }

        context ctx {ev->_target, *this};

        it->second->process(ctx, *ev);

        if (ev->_kind == event_kind::poison_pill) {
            _alive.erase(it);
        }
    }
};

} // namespace

////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<system> create_system()
{
    return std::make_shared<system_impl>();
}

}   // namespace actors
