#include "read_user_input.hpp"

#include <execution/just_stopped.hpp>
#include <execution/just.hpp>
#include <execution/let_value.hpp>
#include <execution/repeat_effect_until.hpp>
#include <execution/sequence.hpp>
#include <execution/sync_wait.hpp>
#include <execution/then.hpp>
#include <execution/upon_error.hpp>

#include <iostream>
#include <set>

namespace {

////////////////////////////////////////////////////////////////////////////////

void print_error(std::exception_ptr ep)
{
    try {
        std::rethrow_exception(ep);
    } catch (std::exception const& ex) {
        std::cerr << "[ERROR] " << ex.what() << '\n';
    }
}

void print_error(std::error_code ec)
{
    std::cerr << "[ERROR] " << ec << '\n';
}

////////////////////////////////////////////////////////////////////////////////

using namespace execution;

constexpr std::string_view help_str =
R"(commands:
    help - print help
    list - print objects
    new NAME - create object with name NAME
    del NAME - delete object with name NAME
    quit - quit from app

)";

struct application
{
    std::set<std::string> _objects;

    auto process(commands::help)
    {
        std::cout << help_str;

        return just();
    }

    auto process(commands::quit)
    {
        return just_stopped();
    }

    auto process(commands::create cmd)
    {
        _objects.emplace(cmd._name);
        return just();
    }

    auto process(commands::remove cmd)
    {
        _objects.erase(cmd._name);
        return just();
    }

    auto process(commands::list)
    {
        std::cout << "{ ";
        for (auto& name: _objects) {
            std::cout << name << " ";
        }
        std::cout << "}\n";

        return just();
    }
};

auto prompt(std::string line)
{
    return just() | then([=] { std::cout << line; });
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main()
{
    application app;

    sequence(
        prompt("$ "),
        read_user_input(std::cin)
            | let_value([&app] (auto cmd) {
                return app.process(std::move(cmd));
              })
            | upon_error([] (auto error) { print_error(error); }))
    | repeat_effect()
    | this_thread::sync_wait();

    return 0;
}
