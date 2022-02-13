#pragma once

#include <type_traits>

namespace NExecution {

///////////////////////////////////////////////////////////////////////////////

template <typename ... Ts>
struct TSignature {};

template <typename TSender>
struct TSenderTraits;

template <typename TSender, typename TReceiver>
using TConnect = typename TSenderTraits<TSender>::template TConnect<TReceiver>;

template <typename TSender, typename TReceiver>
using TSuccess = typename TSenderTraits<TSender>::template TSuccess<TReceiver>;

template <typename TSender, typename TReceiver>
using TFailure = typename TSenderTraits<TSender>::template TFailure<TReceiver>;

template <typename T>
struct TDebug;

namespace NDetails
{

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename S>
struct TInvokeResultImpl;

template <typename F, typename ... Ts>
struct TInvokeResultImpl<F, TSignature<Ts...>>
{
    using TType = std::invoke_result_t<F, Ts...>;
};

}  // namespace NDetails

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename Sig>
using TInvokeResult = typename NDetails::TInvokeResultImpl<F, Sig>::TType;

///////////////////////////////////////////////////////////////////////////////

template <typename TReceiver>
struct TConnectOp
{
    template <typename TSender>
    using TOp = TConnect<TSender, TReceiver>;
};

template <typename F>
struct TInvokeResultOp
{
    template <typename Sig>
    using TOp = TInvokeResult<F, Sig>;
};

template <typename TReceiver>
struct TSuccessOp
{
    template <typename TSender>
    using TOp = TSuccess<TSender, TReceiver>;
};

template <typename TReceiver>
struct TFailureOp
{
    template <typename TSender>
    using TOp = TFailure<TSender, TReceiver>;
};

}   // namespace NExecution
