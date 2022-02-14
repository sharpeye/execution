#pragma once

#include <type_traits>

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct TTypeList;

namespace NTL {
namespace NDetails {

///////////////////////////////////////////////////////////////////////////////

template <typename TL1, typename TL2>
struct TConcat;

template <typename ... Ts1, typename ... Ts2>
struct TConcat<TTypeList<Ts1...>, TTypeList<Ts2...>>
{
    using TType = TTypeList<Ts1..., Ts2...>;
};

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename TL>
struct TMap;

template <typename F, typename ... Ts>
struct TMap<F, TTypeList<Ts...>>
{
    using TType = TTypeList<typename F::template TOp<Ts>...>;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TL>
struct TFlat;

template <typename H, typename ... Ts>
struct TFlat<TTypeList<H, Ts...>>
{
    using TType = TTypeList<H, Ts...>;
};

template <typename H, typename ... Ts>
struct TFlat<TTypeList<TTypeList<H>, Ts...>>
{
    using TType = TTypeList<H, Ts...>;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TL, typename T>
struct TFind;

template <typename T>
struct TFind<TTypeList<>, T>
{
    static constexpr int Index = -1;
};

template <typename ... Ts, typename T>
struct TFind<TTypeList<T, Ts...>, T>
{
    static constexpr int Index = 0;
};

template <typename H, typename ... Ts, typename T>
struct TFind<TTypeList<H, Ts...>, T>
{
    static constexpr int Index = 1 + TFind<TTypeList<Ts...>, T>::Index;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TL, size_t I>
struct TSelect;

template <typename H, typename ... Ts>
struct TSelect<TTypeList<H, Ts...>, 0>
{
    using TType = H;
};

template <typename H, typename ... Ts, size_t I>
struct TSelect<TTypeList<H, Ts...>, I>
{
    using TType = typename TSelect<TTypeList<Ts...>, I - 1>::TType;
};

}   // namespace NDetails

///////////////////////////////////////////////////////////////////////////////

template <typename TL1, typename TL2>
using TConcat = typename NDetails::TConcat<TL1, TL2>::TType;

template <typename F, typename TL>
using TMap = typename NDetails::TMap<F, TL>::TType;

template <typename TL>
using TFlat = typename NDetails::TFlat<TL>::TType;

template <typename TL, typename T>
using TFind = NDetails::TFind<TL, T>;

template <typename TL, size_t I>
using TSelect = typename NDetails::TSelect<TL, I>::TType;

template <typename TL, typename T>
inline constexpr bool Contains = TFind<TL, T>::Index != -1;

}   // namespace NTL

///////////////////////////////////////////////////////////////////////////////

}   // namespace NExecution
