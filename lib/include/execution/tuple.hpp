#pragma once

#include <tuple>
#include <type_traits>

namespace execution {

///////////////////////////////////////////////////////////////////////////////

template<typename ... Ts>
using decayed_tuple_t = std::tuple<std::decay_t<Ts>...>;

}   // namespace execution
