#pragma once

#include <memory>

namespace actors {

////////////////////////////////////////////////////////////////////////////////

enum class actor_id : int {
    null = 0
};

constexpr bool operator == (actor_id lhs, actor_id rhs)
{
    return static_cast<int>(lhs) == static_cast<int>(rhs);
}

constexpr bool operator < (actor_id lhs, actor_id rhs)
{
    return static_cast<int>(lhs) < static_cast<int>(rhs);
}

template <typename T>
T& operator << (T& stream, actor_id id)
{
    stream << ":" << static_cast<int>(id) << ":";
    return stream;
}

////////////////////////////////////////////////////////////////////////////////

enum class event_kind : int {
    null,
    register_actor,
    bootstrap,
    poison_pill,

    custom_event = 100
};

constexpr event_kind create_event_kind(int num)
{
    return static_cast<event_kind>(static_cast<int>(event_kind::custom_event) + num);
}

////////////////////////////////////////////////////////////////////////////////

struct event
{
    event_kind _kind = event_kind::null;

    actor_id _source = actor_id::null;
    actor_id _target = actor_id::null;

    int _cookie = 0;

    event() = default;

    explicit event(event_kind kind)
        : _kind(kind)
    {}

    event(event_kind kind, actor_id source, actor_id target, int cookie)
        : _kind(kind)
        , _source(source)
        , _target(target)
        , _cookie(cookie)
    {}

    event(event_kind kind, actor_id source, actor_id target)
        : event(kind, source, target, 0)
    {}

    virtual ~event() = default;
};

using event_ptr_t = std::unique_ptr<event>;

template <typename T>
T& operator << (T& stream, event const& ev)
{
    stream << "[" << static_cast<int>(ev._kind) << " "
        << ev._source << " -> " << ev._target
        << " #" << ev._cookie << "]";
    return stream;
}

////////////////////////////////////////////////////////////////////////////////

struct poison_pill
    : event
{
    explicit poison_pill(actor_id target, int cookie = 0)
        : event{event_kind::poison_pill, {}, target, cookie}
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct context;

struct actor
{
    virtual ~actor() = default;
    virtual void process(context const& ctx, event& ev) = 0;
};

using actor_ptr_t = std::unique_ptr<actor>;

////////////////////////////////////////////////////////////////////////////////

struct system
{
    virtual ~system() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void send(event_ptr_t ev) = 0;
    virtual actor_id spawn(actor_ptr_t newbie) = 0;

    // conveniences

    template <typename E, typename ... Ts>
    void send(Ts&& ... args)
    {
        send(std::make_unique<E>(std::forward<Ts>(args)...));
    }

    template <typename T, typename ... Ts>
    actor_id spawn(Ts&& ... args)
    {
        return spawn(std::make_unique<T>(std::forward<Ts>(args)...));
    }
};

////////////////////////////////////////////////////////////////////////////////

struct context
{
    actor_id const _self_id;
    system& _system;

    void send(event_ptr_t ev) const
    {
        if (ev->_source == actor_id::null) {
            ev->_source = _self_id;
        }
        _system.send(std::move(ev));
    }

    template <typename E, typename ... Ts>
    void send(Ts&& ... args) const
    {
        send(std::make_unique<E>(std::forward<Ts>(args)...));
    }

    template <typename E, typename ... Ts>
    void reply(event& ev, Ts&& ... args) const
    {
        auto resp = std::make_unique<E>(std::forward<Ts>(args)...);
        resp->_target = ev._source;
        resp->_cookie = ev._cookie;

        send(std::move(resp));
    }

    template <typename T, typename ... Ts>
    actor_id spawn(Ts&& ... args) const
    {
        return _system.spawn(std::make_unique<T>(std::forward<Ts>(args)...));
    }
};

////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<system> create_system();

}   // namespace actors
