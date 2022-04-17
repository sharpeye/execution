#include <execution/meta.hpp>

#include <gtest/gtest.h>

#include <tuple>
#include <variant>

using namespace execution::meta;

////////////////////////////////////////////////////////////////////////////////

TEST(meta, list)
{
    constexpr auto nothing = list<>{};

    static_assert(nothing.size == 0);
    static_assert(list<int>{}.size == 1);
    static_assert(list<int>{}.head == atom<int>{});
    static_assert(list<int>{}.tail == nothing);

    static_assert((nothing | nothing | nothing | nothing) == nothing);

    static_assert((nothing | list<int>{} | nothing | list<char>{}) == list<int, char>{});
    static_assert((atom<int>{} | nothing | list<char>{}) == list<int, char>{});

    auto ls = list<int, char, double>{};

    static_assert(ls.size == 3);

    static_assert(ls[index_t<0>{}] == atom<int>{});
    static_assert(ls[index_t<1>{}] == atom<char>{});
    static_assert(ls[index_t<2>{}] == atom<double>{});

    static_assert(ls[index_t<-1>{}] == atom<double>{});
    static_assert(ls[index_t<-2>{}] == atom<char>{});
    static_assert(ls[index_t<-3>{}] == atom<int>{});

    static_assert(last(ls) == atom<double>{});

    static_assert(find(ls, atom<int>{}) == 0);
    static_assert(find(ls, atom<char>{}) == 1);
    static_assert(find(ls, atom<double>{}) == 2);
    static_assert(find(ls, atom<float>{}) == -1);

    static_assert(iota<3> == list<int_constant_t<0>, int_constant_t<1>, int_constant_t<2>>{});

    static_assert((ls | ls) == list<int, char, double, int, char, double>{});
    static_assert(concat(ls, ls, ls) == (ls | ls | ls));
    static_assert(concat_unique(ls, ls, ls) == ls);

    static_assert(unique(ls | ls) == ls);
    static_assert(unique(nothing) == nothing);

    static_assert(transform(iota<3>, [] <int i> (index_t<i>) {
        return index_t<i + i>{};
    }) == list<int_constant_t<0>, int_constant_t<2>, int_constant_t<4>>{});

    static_assert(
        chain(list<list<int>, list<char>, list<double>>{}).head == atom<int>{}
    );

    static_assert(chain(ls) == ls);
}
