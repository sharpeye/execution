#include "system.hpp"

#include <future>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

////////////////////////////////////////////////////////////////////////////////

struct ping: actors::event
{
    static constexpr auto kind = actors::create_event_kind(1);

    ping(
            actors::actor_id source,
            actors::actor_id target,
            int cookie = 0)
        : event{kind, source, target, cookie}
    {}
};

struct pong: actors::event
{
    static constexpr auto kind = actors::create_event_kind(2);

    pong()
        : event{kind}
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct witget_descr
{
    std::string _name;
    bool _active = false;
};

namespace create_widget {

////////////////////////////////////////////////////////////////////////////////

struct request: actors::event
{
    static constexpr auto kind = actors::create_event_kind(3);

    std::string _name;

    request(actors::actor_id reg, std::string name)
        : event{kind, {}, reg, 0}
        , _name{std::move(name)}
    {}
};

struct response: actors::event
{
    static constexpr auto kind = actors::create_event_kind(4);

    response()
        : event{kind}
    {}
};

}   // namespace create_widget

namespace list_widgets {

////////////////////////////////////////////////////////////////////////////////

struct request: actors::event
{
    static constexpr auto kind = actors::create_event_kind(5);

    explicit request(actors::actor_id reg)
        : event{kind, {}, reg, 0}
    {}
};

struct response: actors::event
{
    static constexpr auto kind = actors::create_event_kind(6);

    std::vector<witget_descr> _widgets;

    explicit response(std::vector<witget_descr> widgets)
        : event{kind}
        , _widgets(std::move(widgets))
    {}
};

}   // namespace list_widgets

namespace describe_widget {

////////////////////////////////////////////////////////////////////////////////

struct request: actors::event
{
    static constexpr auto kind = actors::create_event_kind(7);

    explicit request(actors::actor_id target, int cookie)
        : event{kind, {}, target, cookie}
    {}
};

struct response: actors::event
{
    static constexpr auto kind = actors::create_event_kind(8);

    std::string _name;
    bool _active;

    response(std::string name, bool active)
        : event{kind}
        , _name{std::move(name)}
        , _active{active}
    {}
};

}   // namespace describe_widget

////////////////////////////////////////////////////////////////////////////////

struct widget
    : actors::actor
{
    std::string _name;
    bool _active = false;

    explicit widget(std::string const& name)
        : _name{name}
    {}

    void process(actors::context const& ctx, actors::event& ev) override
    {
        switch (ev._kind) {
            case actors::event_kind::bootstrap: {
                std::random_device r;
                std::default_random_engine e{r()};
                std::uniform_int_distribution<int> d(0, 1);
                _active = d(e) == 1;

                break;
            }

            case describe_widget::request::kind: {
                ctx.reply<describe_widget::response>(ev, _name, _active);
                break;
            }

            case ping::kind: {
                ctx.reply<pong>(ev);
                break;
            }

            default:;
        }
    }
};

struct list_widgets_worker
    : actors::actor
{
    actors::actor_id _reply_to;
    std::vector<actors::actor_id> _widgets;

    std::vector<witget_descr> _results;
    int _requests = 0;

    list_widgets_worker(
            actors::actor_id reply_to,
            std::vector<actors::actor_id> widgets)
        : _reply_to(reply_to)
        , _widgets(std::move(widgets))
    {}

    void process(actors::context const& ctx, actors::event& ev) override
    {
        switch (ev._kind) {
            case actors::event_kind::bootstrap: {
                _results.resize(_widgets.size());
                int i = 0;
                for (auto& w: _widgets) {
                    ctx.send<describe_widget::request>(w, i++);
                    ++_requests;
                }

                if (!_requests) {
                    reply(ctx);
                }

                break;
            }

            case describe_widget::response::kind: {
                auto& resp = static_cast<describe_widget::response&>(ev);
                _results[ev._cookie] = { resp._name, resp._active };

                if (!--_requests) {
                    reply(ctx);
                }
            }

            default:;
        }
    }

    void reply(actors::context const& ctx)
    {
        auto resp = std::make_unique<list_widgets::response>(std::move(_results));
        resp->_target = _reply_to;

        ctx.send(std::move(resp));
    }
};

struct widget_registry
    : actors::actor
{
    std::map<std::string, actors::actor_id> _widgets;

    void process(actors::context const& ctx, actors::event& ev) override
    {
        switch (ev._kind) {
            case create_widget::request::kind: {
                auto& req = static_cast<create_widget::request&>(ev);

                if (!_widgets.contains(req._name)) {
                    _widgets[req._name] = ctx.spawn<widget>(req._name);
                }

                ctx.reply<create_widget::response>(ev);

                break;
            }

            case list_widgets::request::kind: {
                auto& req = static_cast<list_widgets::request&>(ev);

                std::vector<actors::actor_id> widgets;
                for (auto& kv: _widgets) {
                    widgets.push_back(kv.second);
                }

                ctx.spawn<list_widgets_worker>(ev._source, std::move(widgets));

                break;
            }

            default:;
        }
    }
};

struct application
    : actors::actor
{
    std::promise<void> _promise;
    actors::actor_id _registry;

    explicit application(std::promise<void> p)
        : _promise(std::move(p))
    {}

    void process(actors::context const& ctx, actors::event& ev) override
    {
        switch (ev._kind) {
            case actors::event_kind::bootstrap: {
                _registry = ctx.spawn<widget_registry>();
                ctx.send<create_widget::request>(_registry, "foo");
                ctx.send<create_widget::request>(_registry, "bar");
                ctx.send<create_widget::request>(_registry, "baz");

                ctx.send<list_widgets::request>(_registry);

                break;
            }

            case list_widgets::response::kind: {
                auto& resp = static_cast<list_widgets::response&>(ev);

                std::cout << "widgets: ";
                for (auto& [name, on]: resp._widgets) {
                    std::cout << name << ":" << on << " ";
                }
                std::cout << "\n";
                break;
            }

            case actors::event_kind::poison_pill:
                _promise.set_value();
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

        std::promise<void> promise;
        auto future = promise.get_future();

        auto app = system->spawn<application>(std::move(promise));

        std::cin.get();

        system->send<actors::poison_pill>(app);
        future.wait();

        system->stop();
    }
    catch (std::exception const& ex) {
        std::cerr << "[ERROR] " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
