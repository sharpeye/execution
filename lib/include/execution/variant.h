#pragma once

#include "type_list.h"

#include <variant>

namespace NExecution {
namespace NVariant {

///////////////////////////////////////////////////////////////////////////////

template <typename TL>
struct TImpl;

template <typename ... Ts>
struct TImpl<TTypeList<Ts...>>
{
    using TType = std::variant<Ts...>;
};

}  // namespace NVariant

///////////////////////////////////////////////////////////////////////////////

template <typename TL>
using TVariant = typename NVariant::TImpl<TL>::TType;

}   // namespace NExecution
