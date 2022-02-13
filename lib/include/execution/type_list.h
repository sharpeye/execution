#pragma once

#include <type_traits>

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct TTypeList {};

namespace NTypeList
{

///////////////////////////////////////////////////////////////////////////////

template <typename TL1, typename TL2>
struct TAppendImpl;

template <typename ... Ts1, typename ... Ts2>
struct TAppendImpl<TTypeList<Ts1...>, TTypeList<Ts2...>>
{
    using TType = TTypeList<Ts1..., Ts2...>;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TL>
struct THeadImpl;

template <typename H, typename ... Ts>
struct THeadImpl<TTypeList<H, Ts...>>
{
    using TType = H;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TL>
struct TTailImpl;

template <typename H, typename ... Ts>
struct TTailImpl<TTypeList<H, Ts...>>
{
    using TType = TTypeList<Ts...>;
};

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename TL>
struct TApplyImpl;

template <typename F, typename ... Ts>
struct TApplyImpl<F, TTypeList<Ts...>>
{
    using TType = TTypeList<typename F::template TOp<Ts>...>;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TL>
struct TFlatImpl;

template <typename H, typename ... Ts>
struct TFlatImpl<TTypeList<H, Ts...>>
{
    using TType = TTypeList<H, Ts...>;
};

template <typename H, typename ... Ts>
struct TFlatImpl<TTypeList<TTypeList<H>, Ts...>>
{
    using TType = TTypeList<H, Ts...>;
};

}  // namespace NTypeList

///////////////////////////////////////////////////////////////////////////////

template <typename TL1, typename TL2>
using TAppend = typename NTypeList::TAppendImpl<TL1, TL2>::TType;

template <typename TL>
using THead = typename NTypeList::THeadImpl<TL>::TType;

template <typename TL>
using TTail = typename NTypeList::TTailImpl<TL>::TType;

template <typename F, typename TL>
using TApply = typename NTypeList::TApplyImpl<F, TL>::TType;

template <typename TL>
using TFlat = typename NTypeList::TFlatImpl<TL>::TType;

}   // namespace NExecution
