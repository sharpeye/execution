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

using none_t = atom<void>;
static constexpr auto none = none_t{};

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
    static constexpr int size = 0;
};

template <typename H, typename ... Ts>
struct list<H, Ts...>
{
    static constexpr int size = 1 + sizeof ... (Ts);

    static constexpr auto head = atom<H>{};
    static constexpr auto tail = list<Ts...>{};

    template <int index>
    constexpr auto operator [] (index_t<index>) const
    {
        static_assert(
            (0 <= index && index < list::size) ||
            (0 < -index && -index <= list::size),
            "index overflow"
        );

        if constexpr (index == 0 || -index == list::size) {
            return list::head;
        } else {
            if constexpr (index < 0) {
                return list::tail[index_t<list::size + index - 1>{}];
            } else {
                return list::tail[index_t<index - 1>{}];
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

template <template <typename...> typename F>
constexpr auto convert_to = [] <template <typename...> typename U, typename ... Ts>
        (meta::atom<U<Ts...>>) constexpr
    {
        return meta::atom<F<Ts...>>{};
    };

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename H>
constexpr bool operator == (atom<T>, atom<H>)
{
    return std::is_same_v<T, H>;
}

template <typename ... Ts, typename ... Hs>
constexpr bool operator == (list<Ts...>, list<Hs...>)
{
    return std::is_same_v<list<Ts...>, list<Hs...>>;
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename ... Hs>
constexpr auto operator | (list<Ts...>, list<Hs...>)
    -> list<Ts..., Hs...>
{
    return {};
}

template <typename T, typename ... Ts>
constexpr auto operator | (atom<T>, list<Ts...>)
    -> list<T, Ts...>
{
    return {};
}

template <typename T, typename H>
constexpr auto operator | (atom<T>, atom<H>) -> list<T, H>
{
    return {};
}

template <typename ... Ts, typename T>
constexpr auto operator | (list<Ts...>, atom<T>)
    -> list<Ts..., T>
{
    return {};
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename T>
constexpr auto find(list<Ts...> ls, T value)
{
    if constexpr (ls.size == 0) {
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
constexpr auto contains(list<Ts...> ls, T item)
{
    return find(ls, item) != -1;
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
constexpr auto unique(list<Ts...> ls)
{
    if constexpr (ls.size == 0) {
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
constexpr auto transform(list<Ts...> ls, F func)
{
    if constexpr (ls.size == 0) {
        return ls;
    } else {
        return func(ls.head) | transform(ls.tail, func);
    }
}

template <typename ... Ts, typename F>
constexpr auto transform_unique(list<Ts...> ls, F func)
{
    return unique(transform(ls, func));
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename I, typename F>
constexpr auto fold(list<Ts...> ls, I init, F func)
{
    if constexpr (ls.size == 0) {
        return init;
    } else {
        return fold(ls.tail, func(init, ls.head), func);
    }
}

template <typename ... Ts, typename I, typename F>
constexpr auto fold_unique(list<Ts...> ls, I init, F func)
{
    return unique(fold(ls, init, func));
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts, typename T, typename U>
constexpr auto replace(list<Ts...> ls, T old_value, U new_value)
{
    if constexpr (ls.size == 0) {
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
constexpr auto remove(list<Ts...> ls, T value)
{
    if constexpr (ls.size == 0) {
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
constexpr auto iota_impl(std::integer_sequence<int, Is...>)
{
    return list<int_constant_t<Is>...>{};
}

template <int N>
static constexpr auto iota = iota_impl(std::make_integer_sequence<int, N>{});

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
constexpr auto last(list<Ts...> ls)
{
    return ls[index_t<-1>{}];
}

////////////////////////////////////////////////////////////////////////////////

template <typename ... Ls>
constexpr auto concat(Ls ... ls)
{
    return (ls | ...);
}

template <typename ... Ls>
constexpr auto concat_unique(Ls ... ls)
{
    return unique(concat(ls...));
}

////////////////////////////////////////////////////////////////////////////////
// [[a], [b, c], [d]] -> [a, b, c, d]

template <typename ... Ts>
constexpr auto chain(list<Ts...> ls)
{
    return ls;
}

template <typename ... Ls>
constexpr auto chain(list<list<Ls>...>)
{
    return (list<Ls>{} | ...);
}

template <typename ... Ts>
constexpr auto chain_unique(list<Ts...> ls)
{
    return unique(chain(ls));
}

////////////////////////////////////////////////////////////////////////////////

constexpr auto is_any = [] <typename ... Ts>  (list<Ts...>, auto func) constexpr {
    return (func(atom<Ts>{}) || ...);
};

constexpr auto is_all = [] <typename ... Ts>  (list<Ts...>, auto func) constexpr {
    return (func(atom<Ts>{}) && ...);
};

////////////////////////////////////////////////////////////////////////////////

template <typename L1, typename L2, typename F>
constexpr auto zip_transform(L1 l1, L2 l2, F fn)
{
    static_assert(l1.size == l2.size);

    if constexpr (l1.size == 0) {
        return l1;
    } else {
        return fn(l1.head, l2.head) | zip_transform(l1.tail, l2.tail, fn);
    }
}

template <typename L1, typename L2, typename F>
constexpr auto zip_transform_unique(L1 l1, L2 l2, F fn)
{
    return unique(zip_transform(l1, l2, fn));
}

}   // namespace execution::meta
