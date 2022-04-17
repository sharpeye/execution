#pragma once

#include <string>

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
