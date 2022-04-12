#include "system.hpp"

#include <iostream>
#include <string>

namespace {

////////////////////////////////////////////////////////////////////////////////

struct custom_events
{
    static constexpr auto ping = actors::create_event_kind(1);
    static constexpr auto pong = actors::create_event_kind(2);
};

////////////////////////////////////////////////////////////////////////////////

struct ping: actors::event
{
    ping(
            actors::actor_id source,
            actors::actor_id target,
            int cookie = 0)
        : event{custom_events::ping, source, target, cookie}
    {}
};

struct pong: actors::event
{
    pong(
            actors::actor_id source,
            actors::actor_id target,
            int cookie = 0)
        : event{custom_events::pong, source, target, cookie}
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct test_actor
    : actors::actor
{
    std::string _name;

    explicit test_actor(std::string const& name)
        : _name{name}
    {}

    void process(actors::context const& ctx, actors::event& ev) override
    {
        switch (ev._kind) {
            case actors::event_kind::bootstrap:
                std::cout << "hello!\n";
                break;

            case actors::event_kind::poison_pill:
                std::cout << "bye\n";
                break;

            case custom_events::ping: {
                std::cout << "[" << _name << "] " << ctx._self_id << " ping!\n";

                ctx._system.send<pong>(ctx._self_id, ev._source);

                break;
            }

            case custom_events::pong:
                std::cout << "[" << _name << "] " << ctx._self_id << " pong!\n";
                break;

            default:;
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    try {
        auto system = actors::create_system();
        system->start();

        auto foo = system->spawn(std::make_unique<test_actor>("foo"));
        auto bar = system->spawn(std::make_unique<test_actor>("bar"));

        system->send<ping>(foo, bar);
        system->send<ping>(bar, foo);

        system->stop();
    }
    catch (std::exception const& ex) {
        std::cerr << "[ERROR] " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
