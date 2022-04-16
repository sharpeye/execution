#pragma once

#include <execution/sender_traits.hpp>

#include <istream>
#include <string>
#include <system_error>

namespace commands {

////////////////////////////////////////////////////////////////////////////////

struct help {};

struct quit {};

struct list {};

struct create
{
    std::string _name;
};

struct remove
{
    std::string _name;
};

}   // namespace commands
