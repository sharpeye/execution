#pragma once

#include "sender_traits.hpp"
#include "variant.hpp"

#include <tuple>
#include <utility>

namespace execution {
namespace conditional_impl {

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename F, typename S1, typename S2>
struct operation
{
    using init_state_t = std::tuple<F, S1, S2>;

    template <typename S>
    using operation_t = typename execution::sender_traits<S, R>::operation_t;

    using state_t = variant_t<decltype(
        meta::unique(meta::list<
            init_state_t,
            operation_t<S1>,
            operation_t<S2>>{}
        ))>;

    R _receiver;
    state_t _state;

    template <typename R1, typename F1, typename T1, typename T2>
    operation(R1&& receiver, F1&& func, T1&& s1, T2&& s2)
        : _receiver(std::forward<R1>(receiver))
        , _state {
            std::in_place_type<init_state_t>,
            std::forward<F1>(func),
            std::forward<T1>(s1),
            std::forward<T2>(s2)
        }
    {}

    void start() & noexcept
    {
        try {
            auto&& [f, s1, s2] = std::get<init_state_t>(std::move(_state));

            if (std::invoke(std::move(f))) {
                start_alternative(std::move(s1));
            } else {
                start_alternative(std::move(s2));
            }
        }
        catch (...) {
            execution::set_error(std::move(_receiver), std::current_exception());
        }
    }

    template <typename S>
    void start_alternative(S&& s)
    {
        std::decay_t<S> tmp { std::move(s) };

        auto& op = _state.template emplace<operation_t<S>>(
            execution::connect(std::move(tmp), std::move(_receiver))
        );
        execution::start(op);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename S1, typename S2>
struct sender
{
    F _func;
    S1 _s1;
    S2 _s2;

    template <typename R>
    auto connect(R&& receiver) &
    {
        return operation<std::decay_t<R>, F, S1, S2>{
            std::forward<R>(receiver),
            _func,
            _s1,
            _s2
        };
    }

    template <typename R>
    auto connect(R&& receiver) &&
    {
        return operation<std::decay_t<R>, F, S1, S2>{
            std::forward<R>(receiver),
            std::move(_func),
            std::move(_s1),
            std::move(_s2)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct conditional
{
    template <typename F, typename S1, typename S2>
    constexpr auto operator () (F&& func, S1&& s1, S2&& s2) const
    {
        return sender<std::decay_t<F>, std::decay_t<S1>, std::decay_t<S2>> {
            std::forward<F>(func),
            std::forward<S1>(s1),
            std::forward<S2>(s2)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename F, typename S1, typename S2>
struct sender_traits
{
    using operation_t = operation<R, F, S1, S2>;

    static constexpr auto receiver_type = meta::atom<R>{};
    static constexpr auto true_type = meta::atom<S1>{};
    static constexpr auto false_type = meta::atom<S2>{};

    using values_t = decltype(meta::concat_unique(
        traits::sender_values(true_type, receiver_type),
        traits::sender_values(false_type, receiver_type)
    ));

    using errors_t = decltype(meta::concat_unique(
        traits::sender_errors(true_type, receiver_type),
        traits::sender_errors(false_type, receiver_type),
        meta::list<std::exception_ptr>{}
    ));
};

}   // namespace conditional_impl

////////////////////////////////////////////////////////////////////////////////

template <typename R, typename F, typename S1, typename S2>
struct sender_traits<conditional_impl::sender<F, S1, S2>, R>
    : conditional_impl::sender_traits<R, F, S1, S2>
{};

////////////////////////////////////////////////////////////////////////////////

constexpr auto conditional = conditional_impl::conditional{};

}   // namespace execution
