#pragma once

#include "commands.hpp"

#include <execution/sender_traits.hpp>

#include <istream>
#include <string>
#include <system_error>

namespace read_user_input_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R>
struct operation
{
    R _receiver;
    std::istream* _stream;

    void start()
    {
        std::string line;
        if (!std::getline(*_stream, line)) {
            execution::set_error(
                std::move(_receiver),
                std::make_error_code(std::errc::io_error));
            return;
        }

        if (line == "quit" || line == "q") {
            execution::set_value(std::move(_receiver), commands::quit{});
            return;
        }

        if (line == "help" || line == "h") {
            execution::set_value(std::move(_receiver), commands::help{});
            return;
        }

        if (line == "list" || line == "l") {
            execution::set_value(std::move(_receiver), commands::list{});
            return;
        }

        if (line.starts_with("new ")) {
            execution::set_value(
                std::move(_receiver),
                commands::create{line.substr(4)}
            );
            return;
        }

        if (line.starts_with("del ")) {
            execution::set_value(
                std::move(_receiver),
                commands::remove{line.substr(4)}
            );
            return;
        }

        execution::set_error(
            std::move(_receiver),
            std::make_error_code(std::errc::invalid_argument));
    }
};

////////////////////////////////////////////////////////////////////////////////

using execution::signature;

struct sender
{
    template <typename R>
    using operation_t = operation<R>;
    using values_t = execution::meta::list<
        signature<commands::help>,
        signature<commands::quit>,
        signature<commands::list>,
        signature<commands::create>,
        signature<commands::remove>
    >;
    using errors_t = execution::meta::list<std::error_code>;

    std::istream* _stream;

    template <typename R>
    auto connect(R&& receiver) -> operation<std::decay_t<R>>
    {
        return {
            std::forward<R>(receiver),
            _stream
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct read_user_input
{
    sender operator () (std::istream& stream) const
    {
        return { &stream };
    }
};

}   // namespace read_user_input_impl

////////////////////////////////////////////////////////////////////////////////

constexpr auto read_user_input = read_user_input_impl::read_user_input{};
