#pragma once

#include "meta.hpp"

#include <tuple>
#include <type_traits>

namespace execution {

////////////////////////////////////////////////////////////////////////////////

template<typename ... Ts>
using decayed_tuple_t = std::tuple<std::decay_t<Ts>...>;

template <typename L>
struct tuple;

template <typename ... Ts>
struct tuple<meta::list<Ts...>>
{
    using type = std::tuple<Ts...>;
};

template <auto list>
using tuple_t = typename tuple<std::decay_t<decltype(list)>>::type;

}   // namespace execution
