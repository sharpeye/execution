#pragma once

#include "meta.hpp"

#include <variant>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template <typename L>
struct variant;

template <typename ... Ts>
struct variant<meta::list<Ts...>>
{
    using type = std::variant<Ts...>;
};

template <typename L>
using variant_t = typename variant<std::decay_t<L>>::type;

template <auto& ls>
using variant_v = variant_t<decltype(ls)>;

}   // namespace execution
