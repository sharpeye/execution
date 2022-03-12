#include <execution/type_list.h>
#include <gtest/gtest.h>

using namespace NExecution::NTL;

////////////////////////////////////////////////////////////////////////////////

TEST(TypeList, Test)
{
    static_assert(TTypeList<>{}.Length == 0);
    static_assert(TTypeList<int>{}.Length == 1);
    static_assert(TTypeList<int>{}.Head == TItem<int>{});
    static_assert(TTypeList<int>{}.Tail == TTypeList<>{});
}

TEST(TypeList, Concat)
{
    static_assert(
        (TTypeList<>{} | TTypeList<>{} | TTypeList<>{} | TTypeList<>{})
        ==
        TTypeList<>{}
    );

    static_assert(
        (TTypeList<>{} | TTypeList<int>{} | TTypeList<>{} | TTypeList<char>{})
        ==
        TTypeList<int, char>{}
    );

    static_assert(
        (TItem<int>{} | TTypeList<>{} | TTypeList<char>{})
        ==
        TTypeList<int, char>{}
    );
}
