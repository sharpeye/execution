#pragma once

#include "type_list.h"

#include <utility>

namespace NExecution {

////////////////////////////////////////////////////////////////////////////////

template <typename S>
struct TSenderTraits;

template <typename ... Ts>
struct TSignature {};

////////////////////////////////////////////////////////////////////////////////

template <typename S, typename R>
consteval auto SenderErrors(NTL::TItem<S> = {}, NTL::TItem<R> = {})
{
    return TSenderTraits<S>::template ErrorTypes<R>;
}

template <typename S, typename R>
consteval auto SenderValues(NTL::TItem<S> = {}, NTL::TItem<R> = {})
{
    return TSenderTraits<S>::template ValueTypes<R>;
}

template <typename S, typename R>
consteval auto SenderOperation(NTL::TItem<S> = {}, NTL::TItem<R> = {})
{
    return TSenderTraits<S>::template OperationType<R>;
}

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename ... Ts>
consteval auto InvokeResult(NTL::TItem<F>, NTL::TItem<TSignature<Ts...>>)
    -> NTL::TItem<std::invoke_result_t<F, Ts...>>
{
    return {};
}

template <typename F, typename ... Ts>
consteval bool IsNothrowInvocable(NTL::TItem<F>, NTL::TItem<TSignature<Ts...>>)
{
    return std::is_nothrow_invocable_v<F, Ts...>;
}

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename ... Ts>
constexpr void SetValue(R&& receiver, Ts&& ... values)
{
    std::forward<R>(receiver).SetValue(std::forward<Ts>(values)...);
}

template <typename R, typename E>
constexpr void SetError(R&& receiver, E&& error)
{
    std::forward<R>(receiver).SetError(std::forward<E>(error));
}

template <typename R>
constexpr void SetStopped(R&& receiver)
{
    std::forward<R>(receiver).SetStopped();
}

template <typename S, typename R>
constexpr auto Connect(S&& sender, R&& receiver)
{
    return std::forward<S>(sender).Connect(std::forward<R>(receiver));
}

}   // namespace NExecution
