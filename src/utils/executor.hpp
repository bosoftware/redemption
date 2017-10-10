/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Product name: redemption, a FLOSS RDP proxy
Copyright (C) Wallix 2010
Author(s): Jonathan Poelen
*/

#pragma once

#include <vector>
#include <functional>
#include <type_traits>
#include <cassert>

namespace detail
{
    template<int...>
    class seq_int;

    template<class, class>
    struct cat_seq;

    template<int... i1, int... i2>
    struct cat_seq<seq_int<i1...>, seq_int<i2...>>
    {
        using type = seq_int<i1..., sizeof...(i1)+i2...>;
    };

    template<int n>
    struct make_seq_int_impl
    : cat_seq<
        typename make_seq_int_impl<n/2>::type,
        typename make_seq_int_impl<n - n/2>::type
    >{};

    template<>
    struct make_seq_int_impl<0>
    {
        using type = seq_int<>;
    };

    template<>
    struct make_seq_int_impl<1>
    {
        using type = seq_int<0>;
    };

    template<int n>
    using make_seq_int = typename make_seq_int_impl<n>::type;

    template<class S, class... T>
    class tuple_impl;

    template<int, class T>
    struct tuple_elem
    {
        T x;
    };

    template<int... ints, class... Ts>
    struct tuple_impl<seq_int<ints...>, Ts...>
    : tuple_elem<ints, Ts>...
    {
        template<class... Args>
        tuple_impl(Args&&... args)
        : tuple_elem<ints, Ts>{static_cast<Args&&>(args)}...
        {}

        tuple_impl(tuple_impl &&) = default;
        tuple_impl(tuple_impl const &) = default;
        tuple_impl& operator=(tuple_impl &&) = default;
        tuple_impl& operator=(tuple_impl const &) = default;

        template<class F, class... Args>
        auto invoke(F && f, Args&&... args)
        -> decltype(f(
            static_cast<Args&&>(args)...,
            static_cast<tuple_elem<ints, Ts>*>(nullptr)->x...
        ))
        {
            return f(static_cast<Args&&>(args)..., static_cast<tuple_elem<ints, Ts>&>(*this).x...);
        }
    };

    template<class... Ts>
    using tuple = tuple_impl<make_seq_int<int(sizeof...(Ts))>, Ts...>;

    template<class... Args>
    using ctx_arg_type = detail::tuple<typename std::decay<Args>::type...>;
}


#define CXX_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

class ExecutorActionContext;
class ExecutorTimeoutContext;
class ExecutorResult;

struct ExecutorEvent
{
    class any {};
    struct real_deleter
    {
        void (*deleter) (void*);
        void operator()(any* x) const
        {
            deleter(x);
        }
    };
    std::unique_ptr<any, real_deleter> ctx;
    std::function<ExecutorResult(ExecutorActionContext, any&)> on_action;
    std::function<ExecutorResult(ExecutorTimeoutContext, any&)> on_timeout;
    std::function<ExecutorResult(ExecutorActionContext, bool success, any&)> on_exit;

    ExecutorEvent()
    {}

    template<class Ctx, class... Args>
    ExecutorEvent(Ctx*, Args&&... args)
      : ctx(
          reinterpret_cast<any*>(new Ctx(static_cast<Args&&>(args)...)),
          real_deleter{[](void* p){ delete static_cast<Ctx*>(p); }})
    {}

    template<class Ctx>
    ExecutorEvent(Ctx*)
      : ctx(reinterpret_cast<any*>(this), real_deleter{[](void*){ }})
    {}
};

struct ExecutorResult
{
    enum ProcessFunction
    {
        ReplaceAction,
        ReplaceTimeout,
        SubExecutor,
        ExitSuccess,
        ExitFailure,
    };

    ProcessFunction process_function;
    ExecutorEvent event;
};

class Executor;

template<class Ctx>
struct EventInitializer
{
    ExecutorEvent& executor_event;
    Executor& executor;

    template<class F>
    void init_on_action(F&& f);

    template<class F>
    void init_on_exit(F&& f);

    template<class F>
    void init_on_timeout(F&& f);
};

template<class EventCtx, int i, bool Initial>
struct SubExecutorImpl;

template<class EventCtx, class F, bool Initial, int i, int flag>
using NextSubExecutorImpl = typename std::enable_if<
    ((void)static_cast<F*>(nullptr), !(i & flag)),
    SubExecutorImpl<EventCtx, i | flag, Initial>
>::type;

template<class EventCtx, int i, bool Initial>
struct SubExecutorImpl
{
    template<class F>
    CXX_WARN_UNUSED_RESULT
    NextSubExecutorImpl<EventCtx, F, Initial, i, 1/*0b001*/>
    on_action(F&& f) &&
    {
        this->event_initializer.init_on_action(static_cast<F&&>(f));
        return {this->event_initializer};
    }

    template<class F>
    CXX_WARN_UNUSED_RESULT
    NextSubExecutorImpl<EventCtx, F, Initial, i, 2/*0b010*/>
    on_timeout(F&& f) &&
    {
        this->event_initializer.init_on_timeout(static_cast<F&&>(f));
        return {this->event_initializer};
    }

    template<class F>
    CXX_WARN_UNUSED_RESULT
    NextSubExecutorImpl<EventCtx, F, Initial, i, 4/*0b100*/>
    on_exit(F&& f) &&
    {
        this->event_initializer.init_on_exit(static_cast<F&&>(f));
        return {this->event_initializer};
    }

    SubExecutorImpl(EventInitializer<EventCtx> event_initializer) noexcept
      : event_initializer(event_initializer)
    {}

private:
    EventInitializer<EventCtx> event_initializer;
};

#define MK_SubExecutorImpl(i, mem)                                             \
    template<class EventCtx>                                                   \
    struct SubExecutorImpl<EventCtx, i, false>                                 \
    {                                                                          \
        template<class F>                                                      \
        CXX_WARN_UNUSED_RESULT ExecutorResult mem(F&& f) &&                    \
        {                                                                      \
            this->event_initializer.init_##mem(static_cast<F&&>(f));           \
            return {ExecutorResult::SubExecutor, {}};                          \
        }                                                                      \
                                                                               \
        SubExecutorImpl(EventInitializer<EventCtx> event_initializer) noexcept \
        : event_initializer(event_initializer)                                 \
        {}                                                                     \
                                                                               \
    private:                                                                   \
        EventInitializer<EventCtx> event_initializer;                          \
    };                                                                         \
                                                                               \
    template<class EventCtx>                                                   \
    struct SubExecutorImpl<EventCtx, i, true>                                  \
    {                                                                          \
        template<class F>                                                      \
        void mem(F&& f) &&                                                     \
        {                                                                      \
            this->event_initializer.init_##mem(static_cast<F&&>(f));           \
        }                                                                      \
                                                                               \
        SubExecutorImpl(EventInitializer<EventCtx> event_initializer) noexcept \
        : event_initializer(event_initializer)                                 \
        {}                                                                     \
                                                                               \
    private:                                                                   \
        EventInitializer<EventCtx> event_initializer;                          \
    }

MK_SubExecutorImpl(6/*0b110*/, on_action);
MK_SubExecutorImpl(5/*0b101*/, on_timeout);
MK_SubExecutorImpl(3/*0b011*/, on_exit);

#undef MK_SubExecutorImpl


template<class EventCtx, bool Initial>
struct SetSubExecutor
{
    using SubExecutor = SubExecutorImpl<EventCtx, 0, Initial>;

    template<class F>
    CXX_WARN_UNUSED_RESULT
    SubExecutorImpl<EventCtx, 1/*0b001*/, Initial>
    on_action(F&& f) &&
    {
        return SubExecutor{this->event_initializer}.on_action(static_cast<F&&>(f));
    }

    template<class F>
    CXX_WARN_UNUSED_RESULT
    SubExecutorImpl<EventCtx, 2/*0b010*/, Initial>
    on_timeout(F&& f) &&
    {
        return SubExecutor{this->event_initializer}.on_timeout(static_cast<F&&>(f));
    }

    template<class F>
    CXX_WARN_UNUSED_RESULT
    SubExecutorImpl<EventCtx, 4/*0b100*/, Initial> on_exit(F&& f) &&
    {
        return SubExecutor{this->event_initializer}.on_exit(static_cast<F&&>(f));
    }

    explicit SetSubExecutor(EventInitializer<EventCtx> event_initializer) noexcept
      : event_initializer(event_initializer)
    {}

private:
    EventInitializer<EventCtx> event_initializer;
};

template<class EventCtx>
using InitialSubExecutor = SetSubExecutor<EventCtx, true>;

template<class EventCtx>
using SubExecutor = SetSubExecutor<EventCtx, false>;

template<class... Args>
using MakeSubExecutor = SubExecutor<detail::ctx_arg_type<Args...>>;

template<class... Args>
using MakeInitialSubExecutor = InitialSubExecutor<detail::ctx_arg_type<Args...>>;

struct Executor
{
    template<class... Args>
    CXX_WARN_UNUSED_RESULT
    MakeInitialSubExecutor<Args...>
    initial_executor(const char * /*TODO name*/, Args&&... args)
    {
        return MakeInitialSubExecutor<Args...>{
            this->create_ctx_events(static_cast<Args&&>(args)...)};
    }

    bool exec();

private:
    template<class... Args>
    CXX_WARN_UNUSED_RESULT
    MakeSubExecutor<Args...>
    sub_executor(const char * /*TODO name*/, Args&&... args)
    {
        return MakeSubExecutor<Args...>{
            this->create_ctx_events(static_cast<Args&&>(args)...)};
    }

    template<class... Args>
    EventInitializer<detail::ctx_arg_type<Args...>> create_ctx_events(Args&&... args)
    {
        using Ctx = detail::ctx_arg_type<Args...>;
        Ctx * p;
        this->events.emplace_back(p, static_cast<Args&&>(args)...);
        return {this->events.back(), *this};
    }

    friend class ExecutorContext;
    friend class ExecutorActionContext;
    friend class ExecutorTimeoutContext;

    std::vector<ExecutorEvent> events;
};


struct ExecutorContext
{
    enum class Status { Error, Success, };

    CXX_WARN_UNUSED_RESULT ExecutorResult exit(Status status)
    {
        return (status == Status::Success) ? this->exit_on_success() : this->exit_on_error();
    }

    CXX_WARN_UNUSED_RESULT ExecutorResult exit_on_error()
    {
        return {ExecutorResult::ExitFailure, {}};
    }

    CXX_WARN_UNUSED_RESULT ExecutorResult exit_on_success()
    {
        return {ExecutorResult::ExitSuccess, {}};
    }

    template<class F, class... Args>
    CXX_WARN_UNUSED_RESULT
    MakeSubExecutor<Args...>
    sub_executor(const char * name, F&& f, Args&&... args)
    {
        return this->executor.sub_executor(
            name, static_cast<F&&>(f), static_cast<Args&&>(args)...);
    }

    explicit ExecutorContext(Executor& executor) noexcept
      : executor(executor)
    {}

protected:
    Executor & executor;
};


struct ExecutorTimeoutContext : ExecutorContext
{
    template<class F>
    CXX_WARN_UNUSED_RESULT ExecutorResult next_timeout(F&& f)
    {
        auto event_initializer = this->executor.create_ctx_events();
        event_initializer.init_on_timeout(static_cast<F&&>(f));
        return {ExecutorResult::ReplaceTimeout, std::move(event_initializer.executor_event)};
    }

    template<class F>
    void set_action(F&& f)
    {
        this->executor.events.back().on_action = static_cast<F&&>(f);
    }

    explicit ExecutorTimeoutContext(Executor& executor) noexcept
      : ExecutorContext(executor)
    {}
};

struct ExecutorActionContext : ExecutorContext
{
    template<class F>
    CXX_WARN_UNUSED_RESULT ExecutorResult next_action(F&& f)
    {
        auto event_initializer = this->executor.create_ctx_events();
        event_initializer.init_on_action(static_cast<F&&>(f));
        return {ExecutorResult::ReplaceAction, std::move(event_initializer.executor_event)};
    }

    template<class F>
    void set_timeout(F&& f)
    {
        this->executor.events.back().on_timeout = static_cast<F&&>(f);
    }

    explicit ExecutorActionContext(Executor& executor) noexcept
      : ExecutorContext(executor)
    {}
};

bool Executor::exec()
{
    auto process_exit = [this](bool status) {
        while (!this->events.empty()) {
            ExecutorResult r = this->events.back().on_exit(
                ExecutorActionContext{*this}, status, *this->events.back().ctx);
            switch (r.process_function) {
                case ExecutorResult::ExitSuccess:
                    status = true;
                    this->events.pop_back();
                    break;
                case ExecutorResult::ExitFailure:
                    status = false;
                    this->events.pop_back();
                    break;
                case ExecutorResult::ReplaceAction:
                    this->events.pop_back();
                    assert(this->events.size());
                    if (!this->events.empty()) {
                        this->events.back().on_action = std::move(r.event.on_action);
                    }
                    return;
                case ExecutorResult::SubExecutor:
                    this->events.back() = std::move(r.event);
                    return;
                case ExecutorResult::ReplaceTimeout:
                    assert(false);
            }
        }
    };

    ExecutorResult r = this->events.back().on_action(
        ExecutorActionContext{*this}, *this->events.back().ctx);
    switch (r.process_function) {
        case ExecutorResult::ExitSuccess:
            process_exit(true);
            break;
        case ExecutorResult::ExitFailure:
            process_exit(false);
            break;
        case ExecutorResult::ReplaceAction:
            this->events.back().on_action = std::move(r.event.on_action);
            break;
        case ExecutorResult::SubExecutor: this->events.back() = std::move(r.event);
            break;
        case ExecutorResult::ReplaceTimeout:
            assert(false);
    }

    return !this->events.empty();
}


template<class Ctx>
template<class F>
void EventInitializer<Ctx>::init_on_action(F&& f)
{
    this->executor_event.on_action = [f](
        ExecutorActionContext exec_ctx,
        ExecutorEvent::any& any
    ){
        return reinterpret_cast<Ctx&>(any).invoke(f, exec_ctx);
    };
}

template<class Ctx>
template<class F>
void EventInitializer<Ctx>::init_on_exit(F&& f)
{
    this->executor_event.on_exit = [f](
        ExecutorActionContext exec_ctx,
        bool success,
        ExecutorEvent::any& any
    ){
        return reinterpret_cast<Ctx&>(any).invoke(f, exec_ctx, success);
    };
}

template<class Ctx>
template<class F>
void EventInitializer<Ctx>::init_on_timeout(F&& f)
{
    this->executor_event.on_timeout = [f](
        ExecutorTimeoutContext exec_ctx,
        ExecutorEvent::any& any
    ){
        return reinterpret_cast<Ctx&>(any).invoke(f, exec_ctx);
    };
}
