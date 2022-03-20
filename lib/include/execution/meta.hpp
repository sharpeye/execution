#pragma once

#include <type_traits>
#include <utility>

namespace execution::meta {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct atom
{
    using type = T;
};

static constexpr auto none = atom<void>{};

////////////////////////////////////////////////////////////////////////////////

template <int value>
using int_constant_t = std::integral_constant<int, value>;

template <int value>
using index_t = atom<int_constant_t<value>>;

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct list;

template <>
struct list<>
{
    static constexpr std::size_t length = 0;
    static constexpr bool is_empty = true;
};

template <typename H, typename ... Ts>
struct list<H, Ts...>
{
    static constexpr int length = 1 + sizeof ... (Ts);
    static constexpr bool is_empty = false;

    static constexpr auto head = atom<H>{};
    static constexpr auto tail = list<Ts...>{};

    template <int index>
    consteval auto operator [] (index_t<index>) const
    {
        static_assert(
            (0 <= index && index < list::length) ||
            (0 < -index && -index <= list::length),
            "index overflow"
        );

        if constexpr (index == 0 || -index == list::length) {
            return list::head;
        } else {
            if constexpr (index < 0) {
                return list::tail[index_t<list::length + index - 1>{}];
            } else {
                return list::tail[index_t<index - 1>{}];
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

template <template <typename...> typename F>
constexpr auto convert_to = [] <template <typename...> typename U, typename ... Ts>
        (meta::atom<U<Ts...>>) consteval
    {
        return meta::atom<F<Ts...>>{};
    };

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename H>
consteval bool operator == (atom<T>, atom<H>)
{
    return std::is_same_v<T, H>;
}

template <typename ... Ts, typename ... Hs>
consteval bool operator == (list<Ts...>, list<Hs...>)
{
    return std::is_same_v<list<Ts...>, list<Hs...>>;
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename ... Hs>
consteval auto operator | (list<Ts...>, list<Hs...>)
    -> list<Ts..., Hs...>
{
    return {};
}

template <typename T, typename ... Ts>
consteval auto operator | (atom<T>, list<Ts...>)
    -> list<T, Ts...>
{
    return {};
}

template <typename T, typename H>
consteval auto operator | (atom<T>, atom<H>) -> list<T, H>
{
    return {};
}

template <typename ... Ts, typename T>
consteval auto operator | (list<Ts...>, atom<T>)
    -> list<Ts..., T>
{
    return {};
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename T>
consteval auto find(list<Ts...> ls, T value)
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
consteval auto contains(list<Ts...> ls, T item)
{
    return find(ls, item) != -1;
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
consteval auto unique(list<Ts...> ls)
{
    if constexpr (ls.is_empty) {
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

template <typename ... Ts, typename F>
consteval auto transform(list<Ts...> ls, F func)
{
    if constexpr (ls.is_empty) {
        return ls;
    } else {
        return func(ls.head) | transform(ls.tail, func);
    }
}

template <typename ... Ts, typename F>
consteval auto transform_unique(list<Ts...> ls, F func)
{
    return unique(transform(ls, func));
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename I, typename F>
consteval auto fold(list<Ts...> ls, I init, F func)
{
    if constexpr (ls.is_empty) {
        return init;
    } else {
        return fold(ls.tail, func(init, ls.head), func);
    }
}

template <typename ... Ts, typename I, typename F>
consteval auto fold_unique(list<Ts...> ls, I init, F func)
{
    return unique(fold(ls, init, func));
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename T, typename U>
consteval auto replace(list<Ts...> ls, T old_value, U new_value)
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

template <typename ... Ts, typename T>
consteval auto remove(list<Ts...> ls, T value)
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
    return list<int_constant_t<Is>...>{};
}

template <int N>
static constexpr auto iota = iota_impl(std::make_integer_sequence<int, N>{});

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
consteval auto last(list<Ts...> ls)
{
    return ls[index_t<-1>{}];
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

template <typename ... Ts>
consteval auto chain(list<Ts...> ls)
{
    return ls;
}

template <typename ... Ls>
consteval auto chain(list<list<Ls>...>)
{
    return (list<Ls>{} | ...);
}

template <typename ... Ts>
consteval auto chain_unique(list<Ts...> ls)
{
    return unique(chain(ls));
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename F>
consteval bool is_any(list<Ts...> ls, F func)
{
    return (func(atom<Ts>{}) || ...);
}

template <typename ... Ts, typename F>
consteval bool is_all(list<Ts...> ls, F func)
{
    return (func(atom<Ts>{}) && ...);
}

}   // namespace execution::meta
