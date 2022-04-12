#pragma once

#include <memory>

namespace actors {

////////////////////////////////////////////////////////////////////////////////

enum class actor_id : int {
    null = -1
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
    stream << ":" << id << ":";
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
        send(std::make_unique<E>(std::forward(args)...));
    }
};

////////////////////////////////////////////////////////////////////////////////

struct context
{
    actor_id const _self_id;
    system& _system;
};

////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<system> create_system();

}   // namespace actors
