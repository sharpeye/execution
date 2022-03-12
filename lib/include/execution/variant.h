#pragma once

#include "type_list.h"

#include <variant>

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
consteval auto AsVariant(NTL::TTypeList<Ts...>) -> std::variant<Ts...>
{
    return {};
}

}   // namespace NExecution
