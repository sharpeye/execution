#pragma once

#include "meta.hpp"

#include <variant>

namespace execution {

///////////////////////////////////////////////////////////////////////////////

template <typename L>
struct variant;

template <typename ... Ts>
struct variant<meta::list<Ts...>>
{
    using type = std::variant<Ts...>;
};

template <typename L>
using variant_t = typename variant<L>::type;

}   // namespace execution
