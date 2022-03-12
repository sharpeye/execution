#pragma once

#include "sender_traits.h"

#include <exception>
#include <functional>

namespace NExecution {
namespace NThen {

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename R>
struct TReceiver
{
    F Func;
    R Receiver;

    template <typename U>
    TReceiver(F&& func, U&& receiver)
        : Func(std::move(func))
        , Receiver(std::forward<U>(receiver))
    {}

    template <typename ... Ts>
    void SetValue(Ts&& ... values)
    {
        if constexpr (std::is_void_v<std::invoke_result_t<F, Ts...>>) {
            std::invoke(std::move(Func), std::forward<Ts>(values)...);
            NExecution::SetValue(std::move(Receiver));
        } else {
            NExecution::SetValue(
                std::move(Receiver),
                std::invoke(std::move(Func), std::forward<Ts>(values)...)
            );
        }
    }

    template <typename E>
    void SetError(E&& error)
    {
        NExecution::SetError(std::move(Receiver), std::forward<E>(error));
    }

    void SetStopped()
    {
        NExecution::SetStopped(std::move(Receiver));
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
            TReceiver<F, R>{
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
    static constexpr auto FuncType = NTL::TItem<F>{};

    template <typename R>
    static constexpr auto OperationType = SenderOperation<P, TReceiver<F, R>>();

    template <typename R>
    static constexpr auto ValueTypes = NTL::Unique(
        NTL::Transform(
            SenderValues<P, TReceiver<F, R>>(),
            [] (auto sig) {
                auto r = InvokeResult(FuncType, sig);
                if constexpr (r == NTL::TItem<void>{}) {
                    return NTL::TItem<TSignature<>>{};
                } else {
                    return NTL::TItem<TSignature<decltype(r)>>{};
                }
            }
        ));

    template <typename R>
    static constexpr auto ErrorTypes = NTL::Unique(
        NTL::Fold(
            SenderValues<P, TReceiver<F, R>>(),
            SenderErrors<P, TReceiver<F, R>>(),
            [] (auto errors, auto sig) {
                if constexpr (IsNothrowInvocable(FuncType, sig)) {
                    return errors;
                } else {
                    return errors | NTL::TItem<std::exception_ptr>{};
                }
            }));
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
