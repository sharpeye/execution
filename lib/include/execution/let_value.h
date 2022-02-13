#pragma once

#include "sender.h"
#include "type_list.h"
#include "variant.h"

#include <type_traits>

namespace NExecution {
namespace NLetValue {

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct TStagingReceiver
{
    T* Op;

    template <typename ... Ts>
    void OnSuccess(Ts&& ... values)
    {
        Op->OnSuccess(std::forward<Ts>(values)...);
    }

    template <typename ... Ts>
    void OnFailure(Ts&& ... errors)
    {
        Op->OnFailure(std::forward<Ts>(errors)...);
    }

    void OnCancel()
    {
        Op->OnCancel();
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct TOp
{
    using TSelf = TOp<P, F, R>;

    F Factory;
    R Receiver;

    TVariant<
        TAppend<
            TTypeList<
                P,
                TConnect<P, TStagingReceiver<TSelf>>
            >,
            TApply<
                TConnectOp<R>,
                TApply<
                    TInvokeResultOp<F>,
                    TSuccess<P, TStagingReceiver<TSelf>>
                >
            >
        >
    > State;

    template <typename U>
    TOp(P&& predecessor, F&& factory, U&& receiver)
        : State(std::move(predecessor))
        , Factory(std::move(factory))
        , Receiver(std::forward<U>(receiver))
    {}

    void Start()
    {
        auto& p = std::get<0>(State);
        State = p.Connect(TStagingReceiver<TSelf>{this});
        std::get<1>(State).Start();
    }

    template <typename ... Ts>
    void OnSuccess(Ts&& ... values)
    {
        auto s = std::invoke(std::move(Factory), std::forward<Ts>(values)...);
        auto op = s.Connect(std::move(Receiver));
        op.Start();
        State = std::move(op);
    }

    template <typename ... Ts>
    void OnFailure(Ts&& ... errors)
    {
        std::move(Receiver).OnFailure(std::forward<Ts>(errors)...);
    }

    void OnCancel()
    {
        std::move(Receiver).OnCancel();
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct TSender
{
    P Predecessor;
    F SuccessorFactory;

    template <typename U, typename V>
    TSender(U&& p, V&& func)
        : Predecessor(std::forward<U>(p))
        , SuccessorFactory(std::forward<V>(func))
    {}

    template <typename R>
    auto Connect(R&& receiver)
    {
        return TOp<P, F, R>{
            std::move(Predecessor),
            std::move(SuccessorFactory),
            std::forward<R>(receiver)
        };
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename F>
struct TPartial
{
    F SuccessorFactory;

    template <typename U>
    TPartial(U&& func)
        : SuccessorFactory(std::forward<U>(func))
    {}
};

template <typename P, typename F>
auto operator | (P&& predecessor, TPartial<F>&& partial)
{
    return TSender<P, F>{
        std::forward<P>(predecessor),
        std::move(partial.SuccessorFactory)
    };
}

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct TTraits
{
    template <typename R>
    using TConnect = TOp<P, F, R>;

    template <typename R>
    using TSuccess = TFlat<
        TApply<
            TSuccessOp<R>,
            TApply<
                TInvokeResultOp<F>,
                TSuccess<P, TStagingReceiver<TConnect<R>>>
            >
        >
    >;

    template <typename R>
    using TFailure = TAppend<
        NExecution::TFailure<P, R>,
        TApply<
            TFailureOp<R>,
            NExecution::TFailure<P, TStagingReceiver<TConnect<R>>>
        >
    >;
};

}   // namespace NLetValue

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
struct TSenderTraits<NLetValue::TSender<P, F>>
    : NLetValue::TTraits<P, F>
{};

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F>
auto LetValue(P&& predecessor, F&& factory)
{
    return NLetValue::TSender<P, F>{
        std::forward<P>(predecessor),
        std::forward<F>(factory)
    };
}

template <typename F>
auto LetValue(F&& factory)
{
    return NLetValue::TPartial<F>{std::forward<F>(factory)};
}

}   // namespace NExecution
