#pragma once

#include "sender.h"
#include "type_list.h"

#include <functional>
#include <type_traits>

namespace NExecution {
namespace NThen {

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename R>
struct TStagingReceiver
{
    F Func;
    R Receiver;

    template <typename U>
    TStagingReceiver(F&& func, U&& receiver)
        : Func(std::move(func))
        , Receiver(std::forward<U>(receiver))
    {}

    template <typename ... Ts>
    void OnSuccess(Ts&& ... values)
    {
        if constexpr (std::is_same_v<void, std::invoke_result_t<F, Ts...>>) {
            std::invoke(std::move(Func), std::forward<Ts>(values)...);
            NExecution::Success(std::move(Receiver));
        } else {
            NExecution::Success(
                std::move(Receiver),
                std::invoke(std::move(Func), std::forward<Ts>(values)...)
            );
        }
    }

    template <typename E>
    void OnFailure(E&& error)
    {
        NExecution::Failure(
            std::move(Receiver),
            std::forward<E>(error)
        );
    }

    void OnCancel()
    {
        NExecution::Cancel(std::move(Receiver));
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct TSender
{
    P Predecessor;
    F Func;

    template <typename U, typename V>
    TSender(U&& p, V&& func)
        : Predecessor(std::forward<U>(p))
        , Func(std::forward<V>(func))
    {}

    template <typename R>
    auto Connect(R&& receiver)
    {
        return NExecution::Connect(
            std::move(Predecessor),
            TStagingReceiver<F, R>{
                std::move(Func), std::forward<R>(receiver)
            });
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename F>
struct TPartial
{
    F Func;

    template <typename U>
    TPartial(U&& func)
        : Func(std::forward<U>(func))
    {}
};

template <typename P, typename F>
auto operator | (P&& predecessor, TPartial<F>&& partial)
{
    return TSender<P, F>{
        std::forward<P>(predecessor),
        std::move(partial.Func)
    };
}

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct TTraits
{
    template <typename R>
    using TConnect = TConnectResult<P, TStagingReceiver<F, R>>;

    template <typename R>
    using TValues = NTL::TMap<TInvokeResult<F>, TSenderValues<P, R>>;

    template <typename R>
    using TErrors = TSenderErrors<P, R>;
};

}   // namespace NThen

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct TSenderTraits<NThen::TSender<P, F>>
    : NThen::TTraits<P, F>
{};

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
auto Then(P&& predecessor, F&& func)
{
    return NThen::TSender<P, F>(
        std::forward<P>(predecessor),
        std::forward<F>(func)
    );
}

template <typename F>
auto Then(F&& func)
{
    return NThen::TPartial<F>{std::forward<F>(func)};
}

}   // namespace NExecution
