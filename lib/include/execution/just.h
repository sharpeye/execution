#pragma once

#include "sender_traits.h"
#include "type_list.h"

#include <tuple>
#include <type_traits>

namespace NExecution {
namespace NJust {

///////////////////////////////////////////////////////////////////////////////

template <typename R, typename T>
struct TOp
{
    R Receiver;
    T Values;

    template <typename U>
    TOp(U&& receiver, T&& values)
        : Receiver(std::forward<U>(receiver))
        , Values(std::move(values))
    {}

    void Start()
    {
        std::apply(
            [this] (auto&& ... values) {
                NExecution::SetValue(
                    std::move(Receiver),
                    std::move(values)...
                );
            },
            std::move(Values)
        );
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct TSender
{
    using TTuple = std::tuple<Ts...>;

    TTuple Values;

    template <typename ... Us>
    explicit TSender(Us&& ... values)
        : Values(std::forward<Us>(values)...)
    {}

    template <typename R>
    auto Connect(R&& receiver)
    {
        return TOp<R, TTuple>{
            std::forward<R>(receiver),
            std::move(Values)
        };
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct TTraits
{
    template <typename R>
    static constexpr auto OperationType = NTL::TItem<TOp<R, std::tuple<Ts...>>>{};

    template <typename R>
    static constexpr auto ValueTypes = NTL::TTypeList<TSignature<Ts...>>{};

    template <typename R>
    static constexpr auto ErrorTypes = NTL::TTypeList<>{};
};

}   // namespace NJust

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct TSenderTraits<NJust::TSender<Ts...>>
    : NJust::TTraits<Ts...>
{};

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
auto Just(Ts&& ... values)
{
    return NJust::TSender<std::decay_t<Ts>...>(std::forward<Ts>(values)...);
}

}   // namespace NExecution
