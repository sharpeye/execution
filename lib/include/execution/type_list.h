#pragma once

#include <type_traits>

namespace NExecution::NTL {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct TItem
{
    using TType = T;
};

struct TNone {};

static constexpr auto None = TItem<TNone>{};

template <typename ... Ts>
struct TTypeList;

template <>
struct TTypeList<>
{
    static constexpr std::size_t Length = 0;
    static constexpr bool IsEmpty = true;
};

template <typename H, typename ... Ts>
struct TTypeList<H, Ts...>
{
    static constexpr std::size_t Length = 1 + sizeof ... (Ts);
    static constexpr bool IsEmpty = false;

    static constexpr TItem<H> Head = {};
    static constexpr TTypeList<Ts...> Tail = {};
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename H>
consteval bool operator == (TItem<T>, TItem<H>)
{
    return std::is_same_v<T, H>;
}

template <typename ... Ts, typename ... Hs>
consteval bool operator == (TTypeList<Ts...>, TTypeList<Hs...>)
{
    return (std::is_same_v<Ts, Hs> && ...);
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename ... Hs>
consteval auto operator | (TTypeList<Ts...>, TTypeList<Hs...>)
    -> TTypeList<Ts..., Hs...>
{
    return {};
}

template <typename T, typename ... Ts>
consteval auto operator | (TItem<T>, TTypeList<Ts...>)
    -> TTypeList<T, Ts...>
{
    return {};
}

template <typename T, typename H>
consteval auto operator | (TItem<T>, TItem<H>) -> TTypeList<T, H>
{
    return {};
}

template <typename ... Ts, typename T>
consteval auto operator | (TTypeList<Ts...>, TItem<T>)
    -> TTypeList<Ts..., T>
{
    return {};
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
consteval int Find(TTypeList<> haystack, T)
{
    return -1;
}

template <typename ... Ts, typename T>
consteval auto Find(TTypeList<Ts...> haystack, T needle)
{
    if constexpr (haystack.Head == needle) {
        return 0;
    } else {
        const int i = Find(haystack.Tail, needle);
        if (i == -1) {
            return -1;
        }
        return 1 + i;
    }
}

template <typename ... Ts, typename T>
consteval auto Contains(TTypeList<Ts...> l, T item)
{
    return Find(l, item) != -1;
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename F>
consteval auto Transform(TTypeList<Ts...> l, F func)
{
    if constexpr (l.IsEmpty) {
        return l;
    } else {
        return func(l.Head) | Transform(l.Tail, func);
    }
}

////////////////////////////////////////////////////////////////////////////////

consteval auto Unique(TTypeList<> l)
{
    return l;
}

template <typename TL>
consteval auto Unique(TL l)
{
    if constexpr (Contains(l.Tail, l.Head)) {
        return Unique(l.Tail);
    } else {
        return l.Head | Unique(l.Tail);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename TL, typename I, typename F>
consteval auto Fold(TL l, I init, F func)
{
    if constexpr (l.IsEmpty) {
        return init;
    } else {
        return Fold(l.Tail, func(init, l.Head), func);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename TL, typename S, typename D>
consteval auto Replace(TL l, S s, D d)
{
    if constexpr (l.IsEmpty) {
        return l;
    } else {
        auto tail = Replace(l.Tail, s, d);

        if constexpr (l.Head == s) {
            return d | tail;
        } else {
            return l.Head | tail;
        }
    }
}

}   // namespace NExecution::NTL
