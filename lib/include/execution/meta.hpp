#pragma once

#include <type_traits>
#include <utility>

namespace execution::meta {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct atom_t
{
    using type = T;
};

template <typename T>
static constexpr auto atom = atom_t<T>{};

static constexpr auto none = atom<void>;

////////////////////////////////////////////////////////////////////////////////

template <int I>
using int_constant_t = std::integral_constant<int, I>;

template <int I>
using number_t = atom_t<int_constant_t<I>>;

template <int I>
static constexpr auto number = number_t<I>{};

template <int I, int J>
constexpr auto operator + (number_t<I>, number_t<J>)
{
    return number<I + J>;
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct list_t;

template <>
struct list_t<>
{
    static constexpr std::size_t length = 0;
    static constexpr bool is_empty = true;
};

template <typename H, typename ... Ts>
struct list_t<H, Ts...>
{
    static constexpr int length = 1 + sizeof ... (Ts);
    static constexpr bool is_empty = false;

    static constexpr auto head = atom<H>;
    static constexpr auto tail = list_t<Ts...>{};

    template <int I>
    consteval auto operator [] (number_t<I>) const
    {
        static_assert(
            (0 <= I && I < list_t::length) ||
            (0 < -I && -I <= list_t::length),
            "number overflow"
        );

        if constexpr (I == 0 || -I == list_t::length) {
            return list_t::head;
        } else {
            if constexpr (I < 0) {
                return list_t::tail[number<list_t::length + I - 1>];
            } else {
                return list_t::tail[number<I - 1>];
            }
        }
    }
};

template <typename ... Ts>
static constexpr auto list = list_t<Ts...>{};

static constexpr auto nothing = list_t<>{};

////////////////////////////////////////////////////////////////////////////////

template <template <typename...> typename F, typename ... Ts>
constexpr auto convert_to(list_t<Ts...>)
{
    return F<Ts...>{};
}

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename H>
consteval bool operator == (atom_t<T>, atom_t<H>)
{
    return std::is_same_v<T, H>;
}

template <typename ... Ts, typename ... Hs>
consteval bool operator == (list_t<Ts...>, list_t<Hs...>)
{
    return (std::is_same_v<Ts, Hs> && ...);
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename ... Hs>
consteval auto operator | (list_t<Ts...>, list_t<Hs...>)
    -> list_t<Ts..., Hs...>
{
    return {};
}

template <typename T, typename ... Ts>
consteval auto operator | (atom_t<T>, list_t<Ts...>)
    -> list_t<T, Ts...>
{
    return {};
}

template <typename T, typename H>
consteval auto operator | (atom_t<T>, atom_t<H>) -> list_t<T, H>
{
    return {};
}

template <typename ... Ts, typename T>
consteval auto operator | (list_t<Ts...>, atom_t<T>)
    -> list_t<Ts..., T>
{
    return {};
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename T>
consteval auto find(list_t<Ts...> ls, T value)
{
    if constexpr (ls.is_empty) {
        return -1;
    } else {
        if constexpr (ls.head == value) {
            return 0;
        } else {
            const int i = find(ls.tail, value);
            if (i == -1) {
                return -1;
            }
            return 1 + i;
        }
    }
}

template <typename ... Ts, typename T>
consteval auto contains(list_t<Ts...> ls, T item)
{
    return find(ls, item) != -1;
}

////////////////////////////////////////////////////////////////////////////////

template <typename L>
consteval auto unique(L ls)
{
    if constexpr (ls.is_empty ) {
        return ls;
    } else {
        auto tail = unique(ls.tail);

        if constexpr (contains(ls.tail, ls.head)) {
            return tail;
        } else {
            return ls.head | tail;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename L, typename F>
consteval auto transform(L ls, F func)
{
    if constexpr (ls.is_empty) {
        return ls;
    } else {
        return func(ls.head) | transform(ls.tail, func);
    }
}

template <typename L, typename F>
consteval auto transform_unique(L ls, F func)
{
    return unique(transform(ls, func));
}

////////////////////////////////////////////////////////////////////////////////

template <typename L, typename I, typename F>
consteval auto fold(L ls, I init, F func)
{
    if constexpr (ls.is_empty) {
        return init;
    } else {
        return fold(ls.tail, func(init, ls.head), func);
    }
}

template <typename L, typename I, typename F>
consteval auto fold_unique(L ls, I init, F func)
{
    return unique(fold(ls, init, func));
}

////////////////////////////////////////////////////////////////////////////////

template <typename L, typename T, typename U>
consteval auto replace(L ls, T old_value, U new_value)
{
    if constexpr (ls.is_empty) {
        return ls;
    } else {
        auto tail = replace(ls.tail, old_value, new_value);

        if constexpr (ls.head == old_value) {
            return new_value | tail;
        } else {
            return ls.head | tail;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename L, typename T>
consteval auto remove(L ls, T value)
{
    if constexpr (ls.is_empty) {
        return ls;
    } else {
        auto tail = remove(ls.tail, value);

        if constexpr (ls.head == value) {
            return tail;
        } else {
            return ls.head | tail;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <int ... Is>
consteval auto iota_impl(std::integer_sequence<int, Is...>)
{
    return list_t<int_constant_t<Is>...>{};
}

template <int N>
static constexpr auto iota = iota_impl(std::make_integer_sequence<int, N>{});

////////////////////////////////////////////////////////////////////////////////

template <typename L>
consteval auto last(L ls)
{
    return ls[number<-1>];
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ls>
consteval auto concat(Ls ... ls)
{
    return (ls | ...);
}

template <typename ... Ls>
consteval auto concat_unique(Ls ... ls)
{
    return unique(concat(ls...));
}

////////////////////////////////////////////////////////////////////////////////
// [[a], [b, c], [d]] -> [a, b, c, d]

template <typename ... Ls>
consteval auto chain(list_t<Ls...>)
{
    return (Ls{} | ...);
}

template <typename L>
consteval auto chain_unique(L ls)
{
    return unique(chain(ls));
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename F>
consteval bool is_any(list_t<Ts...> ls, F func)
{
    return (func(atom<Ts>) || ...);
}

template <typename ... Ts, typename F>
consteval bool is_all(list_t<Ts...> ls, F func)
{
    return (func(atom<Ts>) && ...);
}

}   // namespace execution::meta
