#pragma once

#include "sender.h"
#include "type_list.h"

#include <tuple>
#include <type_traits>

namespace NExecution {
namespace NJust {

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
using TTuple = std::tuple<std::decay_t<Ts>...>;

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
                NExecution::Success(
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
    TTuple<Ts...> Values;

    template <typename ... Us>
    explicit TSender(Us&& ... values)
        : Values(std::forward<Us>(values)...)
    {}

    template <typename R>
    auto Connect(R&& receiver)
    {
        return TOp<R, TTuple<Ts...>>{
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
    using TConnect = TOp<R, TTuple<Ts...>>;

    template <typename R>
    using TValues = TTypeList<
        TSignature<Ts...>
    >;

    template <typename R>
    using TErrors = TTypeList<>;
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
    return NJust::TSender<Ts...>(std::forward<Ts>(values)...);
}

}   // namespace NExecution
