#pragma once

#include "forward_receiver.h"
#include "tuple.h"
#include "variant.h"

#include <functional>

namespace NExecution {
namespace NLetValue {

///////////////////////////////////////////////////////////////////////////////

template <typename P, typename F, typename R>
struct TOp
{
    using TSelf = TOp<P, F, R>;

    static constexpr auto PredecessorType = NTL::TItem<P>{};
    static constexpr auto FactoryType = NTL::TItem<F>{};
    static constexpr auto ReceiverType = NTL::TItem<R>{};

    static constexpr auto PredecessorReceiverType = NTL::TItem<TForwardReceiver<TSelf>>{};
    static constexpr auto PredecessorOperationType = SenderOperation(
        PredecessorType,
        PredecessorReceiverType);
    static constexpr auto PredecessorValuesTypes = SenderValues(
        PredecessorType,
        PredecessorReceiverType);

    static constexpr auto SuccessorTypes = NTL::Transform(
        PredecessorValuesTypes,
        [] (auto sig) {
            return InvokeResult(FactoryType, sig);
        });

    using TPredecessor = P;

    using TState = decltype(AsVariant(
          NTL::None
        | PredecessorType
        | PredecessorOperationType
        | NTL::Transform(SuccessorTypes, [] (auto s) {
            return SenderOperation(s, ReceiverType);
        })
    ));

    using TValues = decltype(AsVariant(
          NTL::None
        | NTL::Transform(PredecessorValuesTypes, [] (auto sig) {
            return AsTuple(sig);
        })
    ));

    F Factory;
    R Receiver;
    TValues Values;
    TState State;

    template <typename U>
    TOp(P&& predecessor, F&& factory, U&& receiver)
        : Factory(std::move(factory))
        , Receiver(std::forward<U>(receiver))
        , State(std::in_place_type<TPredecessor>, std::move(predecessor))
    {}

    void Start()
    {
        using TOperation = typename decltype(PredecessorOperationType)::TType;
        using TReceiver = typename decltype(PredecessorReceiverType)::TType;

        auto& op = State.template emplace<TOperation>(
            NExecution::Connect(
                std::get<TPredecessor>(std::move(State)),
                TReceiver{this}
            ));

        op.Start();
    }

    template <typename ... Ts>
    void SetValue(Ts&& ... values)
    {
        using TTuple = std::tuple<std::decay_t<Ts>...>;
        using TOperation = typename decltype(
            SenderOperation(
                InvokeResult(FactoryType, NTL::TItem<TSignature<Ts...>>{}),
                ReceiverType
            ))::TType;

        auto& tuple = Values.template emplace<TTuple>(
            std::forward<Ts>(values)...);

        auto& op = State.template emplace<TOperation>(NExecution::Connect(
            std::apply(std::move(Factory), tuple),
            std::move(Receiver)
        ));

        op.Start();
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
    static constexpr auto PredecessorType = NTL::TItem<P>{};
    static constexpr auto FactoryType = NTL::TItem<F>{};

    template <typename R>
    static constexpr auto OperationType = NTL::TItem<TOp<P, F, R>>{};

    template <typename R>
    static constexpr auto PredecessorReceiverType = NTL::TItem<
        TForwardReceiver<TOp<P, F, R>>>{};

    template <typename R>
    static constexpr auto SuccessorTypes = NTL::Transform(
        SenderValues(PredecessorType, NTL::TItem<R>{}),
        [] (auto sig) {
            return InvokeResult(FactoryType, sig);
        }
    );

    template <typename R>
    static constexpr auto ValueTypes = NTL::Unique(
        NTL::Fold(
            NTL::Transform(
                SuccessorTypes<R>,
                [] (auto s) {
                    return SenderValues(s, NTL::TItem<R>{});
                }
            ),
            NTL::TTypeList<>{},
            [] (auto x, auto y) {
                return x | y;
            }
        )
    );

    template <typename R>
    static constexpr auto ErrorTypes = NTL::Unique(
          SenderErrors(PredecessorType, PredecessorReceiverType<R>)
        | NTL::Transform(SuccessorTypes<R>, [] (auto s) {
            return SenderErrors(s, NTL::TItem<R>{});
        })
    );
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
