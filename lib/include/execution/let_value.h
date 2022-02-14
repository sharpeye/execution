#pragma once

#include "sender.h"
#include "type_list.h"
#include "variant.h"

#include <functional>
#include <type_traits>

namespace NExecution {
namespace NLetValue {

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct TPredReceiver
{
    T* Op;

    template <typename ... Ts>
    void OnSuccess(Ts&& ... values)
    {
        Op->OnSuccess(std::forward<Ts>(values)...);
    }

    template <typename E>
    void OnFailure(E&& error)
    {
        Op->OnFailure(std::forward<E>(error));
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
    using TPredReceiver = TPredReceiver<TOp<P, F, R>>;

    using TSuccessors = NTL::TMap<
        TInvokeResult<F>,
        TSenderValues<P, TPredReceiver>
    >;

    // List of possible types
    using TState = TVariant<
        NTL::TConcat<
            TTypeList<
                P,
                TConnectResult<P, TPredReceiver>
            >,
            NTL::TMap<TConnectResult<R>, TSuccessors>
        >>;

    TState State;
    F Factory;
    R Receiver;

    template <typename U>
    TOp(P&& predecessor, F&& factory, U&& receiver)
        : State(std::move(predecessor))
        , Factory(std::move(factory))
        , Receiver(std::forward<U>(receiver))
    {}

    void Start()
    {
        auto op = NExecution::Connect(
            State.template Extract<P>(),
            TPredReceiver{this}
        );
        op.Start();

        State.Replace(std::move(op));
    }

    template <typename ... Ts>
    void OnSuccess(Ts&& ... values)
    {
        auto op = NExecution::Connect(
            std::invoke(std::move(Factory), std::forward<Ts>(values)...),
            std::move(Receiver)
        );
        op.Start();

        State.Replace(std::move(op));
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
    using TConnect = TPredReceiver<TOp<P, F, R>>;

    template <typename R>
    using TSuccessors = NTL::TMap<
        TInvokeResult<F>,
        TSenderValues<P, TConnect<R>>
    >;

    template <typename R>
    using TValues = NTL::TFlat<
        NTL::TMap<TSenderValues<R>, TSuccessors<R>>
    >;

    template <typename R>
    using TErrors = NTL::TConcat<
        // Fails on predecessor
        TSenderErrors<P, TConnect<R>>,
        // Fails on successor
        NTL::TMap<TSenderErrors<R>, TSuccessors<R>>
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
