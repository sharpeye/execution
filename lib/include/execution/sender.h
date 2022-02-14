#pragma once

#include <type_traits>
#include <utility>

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct TDebug;

template <typename ... Ts>
struct TSignature {};

struct TPlaceholder {};

template <typename TSender>
struct TSenderTraits;

namespace NDetails {

///////////////////////////////////////////////////////////////////////////////

template <typename TSender, typename TReceiver>
struct TConnectResult
{
    using TType = typename TSenderTraits<TSender>::template TConnect<TReceiver>;
};

template <typename TReceiver>
struct TConnectResult<TReceiver, TPlaceholder>
{
    struct TType
    {
        template <typename TSender>
        using TOp = typename TConnectResult<TSender, TReceiver>::TType;
    };
};

///////////////////////////////////////////////////////////////////////////////

template <typename TSender, typename TReceiver>
struct TSenderValues
{
    using TType = typename TSenderTraits<TSender>::template TValues<TReceiver>;
};

template <typename TReceiver>
struct TSenderValues<TReceiver, TPlaceholder>
{
    struct TType
    {
        template <typename TSender>
        using TOp = typename TSenderValues<TSender, TReceiver>::TType;
    };
};

///////////////////////////////////////////////////////////////////////////////

template <typename TSender, typename TReceiver>
struct TSenderErrors
{
    using TType = typename TSenderTraits<TSender>::template TErrors<TReceiver>;
};

template <typename TReceiver>
struct TSenderErrors<TReceiver, TPlaceholder>
{
    struct TType
    {
        template <typename TSender>
        using TOp = typename TSenderErrors<TSender, TReceiver>::TType;
    };
};

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename S>
struct TInvokeResult;

template <typename F, typename ... Ts>
struct TInvokeResult<F, TSignature<Ts...>>
{
    using TType = std::invoke_result_t<F, Ts...>;
};

template <typename F>
struct TInvokeResult<F, TPlaceholder>
{
    struct TType
    {
        template <typename Sig>
        using TOp = typename TInvokeResult<F, Sig>::TType;
    };
};

}  // namespace NDetails

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename Sig = TPlaceholder>
using TInvokeResult = typename NDetails::TInvokeResult<F, Sig>::TType;

template <typename X, typename Y = TPlaceholder>
using TConnectResult = typename NDetails::TConnectResult<X, Y>::TType;

template <typename X, typename Y = TPlaceholder>
using TSenderValues = typename NDetails::TSenderValues<X, Y>::TType;

template <typename X, typename Y = TPlaceholder>
using TSenderErrors = typename NDetails::TSenderErrors<X, Y>::TType;

///////////////////////////////////////////////////////////////////////////////

template <typename S, typename R>
auto Connect(S&& sender, R&& receiver)
{
    return std::forward<S>(sender).Connect(std::forward<R>(receiver));
}

template <typename R, typename ... Ts>
void Success(R&& receiver, Ts&& ... values)
{
    std::forward<R>(receiver).OnSuccess(std::forward<Ts>(values)...);
}

template <typename R, typename E>
void Failure(R&& receiver, E&& error)
{
    std::forward<R>(receiver).OnFailure(std::forward<E>(error));
}

template <typename R>
void Cancel(R&& receiver)
{
    std::forward<R>(receiver).OnCancel();
}

}   // namespace NExecution
