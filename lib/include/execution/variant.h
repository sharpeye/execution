#pragma once

#include "type_list.h"

#include <algorithm>
#include <memory>
#include <type_traits>

namespace NExecution {
namespace NVariant {

///////////////////////////////////////////////////////////////////////////////

template <typename TL>
class TImpl;

template <typename ... Ts>
class TImpl<TTypeList<Ts...>>
{
    using TL = TTypeList<Ts...>;

private:
    alignas(Ts...) std::byte Storage[std::max({sizeof(Ts)...})];
    int Index = -1;

public:
    template <typename T, typename = std::enable_if_t<NTL::Contains<TL, T>, void>>
    explicit TImpl(T&& item)
    {
        Construct<T>(std::forward<T>(item));
    }

    TImpl(TImpl&& rhs)
    {
        if (rhs.Index != -1) {
            MoveFrom<0>(rhs);
        }
    }

    TImpl(const TImpl& rhs) = delete;

    TImpl& operator = (const TImpl& rhs) = delete;
    TImpl& operator = (TImpl&& rhs) = delete;

    ~TImpl()
    {
        Cleanup();
    }

    template <typename T>
    auto Replace(T&& item) -> std::enable_if_t<NTL::Contains<TL, T>, void>
    {
        Cleanup();
        Construct<T>(std::forward<T>(item));
    }

    template <typename T>
    auto Get() -> std::enable_if_t<NTL::Contains<TL, T>, T&>
    {
        return *reinterpret_cast<T*>(Storage);
    }

    template <typename T>
    auto Extract() -> std::enable_if_t<NTL::Contains<TL, T>, T>
    {
        return std::move(Get<T>());
    }

    int GetIndex() const
    {
        return Index;
    }

private:
    template <typename T, typename ... Us>
    auto Construct(Us&& ... args) -> std::enable_if_t<NTL::Contains<TL, T>, void>
    {
        Index = NTL::TFind<TL, T>::Index;

        std::construct_at(
            reinterpret_cast<T*>(Storage),
            std::forward<Us>(args)...
        );
    }

    template <int I>
    void Destroy()
    {
        if (I == Index) {
            using TItem = NTL::TSelect<TL, I>;
            std::destroy_at(reinterpret_cast<TItem*>(Storage));
            Index = -1;

            return;
        }

        if constexpr (I + 1 < sizeof ... (Ts)) {
            Destroy<I + 1>();
        }
    }

    void Cleanup()
    {
        if (Index != -1) {
            Destroy<0>();
        }
    }

    template <int I>
    void MoveFrom(TImpl& rhs)
    {
        if (I == rhs.Index) {
            using TItem = NTL::TSelect<TL, I>;

            std::construct_at(
                reinterpret_cast<TItem*>(Storage),
                rhs.template Extract<TItem>()
            );

            Index = I;

            return;
        }

        if constexpr (I + 1 < sizeof ... (Ts)) {
            MoveFrom<I + 1>(rhs);
        }
    }
};

}  // namespace NVariant

///////////////////////////////////////////////////////////////////////////////

template <typename TL>
using TVariant = typename NVariant::TImpl<TL>;

}   // namespace NExecution
