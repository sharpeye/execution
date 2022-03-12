#pragma once

#include "type_list.h"

#include <tuple>

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, template <typename ...> typename F>
constexpr auto AsTuple(NTL::TItem<F<Ts...>>) -> NTL::TItem<std::tuple<Ts...>>
{
    return {};
}

}   // namespace NExecution
