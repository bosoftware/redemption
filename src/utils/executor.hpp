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
#include <tuple>
#include <cassert>


#define CXX_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

class ExecutorActionContext;
class ExecutorTimeoutContext;
class ExecutorResult;

struct ExecutorEvent
{
    class any{};
    struct real_deleter
    {
        void (*deleter) (void*) = +[](void*){};
        void operator()(any* x) const
        {
            deleter(x);
        }
    };
    std::unique_ptr<any, real_deleter> ctx;
    std::function<ExecutorResult(ExecutorActionContext)> on_action;
    std::function<ExecutorResult(ExecutorTimeoutContext)> on_timeout;
    std::function<ExecutorResult(ExecutorActionContext, bool success)> on_exit;

    ExecutorEvent()
    {}

    template<class... Args>
    ExecutorEvent(Args&&... args)
      : ExecutorEvent(any{}, std::make_tuple(static_cast<Args&&>(args)...))
    {}

private:
    template<class T>
    ExecutorEvent(any, T&& t)
      : ctx(static_cast<T&&>(t), +[](void* p){ delete static_cast<T&>(p); })
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

struct ExecutorEventInitializer
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

template<int i, bool Initial>
struct SubExecutorImpl;

template<class F, bool Initial, int i, int flag>
using NextSubExecutorImpl = typename std::enable_if<
    ((void)static_cast<F*>(nullptr), !(i & flag)),
    SubExecutorImpl<i | flag, Initial>
>::type;

template<int i, bool Initial>
struct SubExecutorImpl
{
    template<class F>
    CXX_WARN_UNUSED_RESULT NextSubExecutorImpl<F, Initial, i, 1/*0b001*/> on_action(F&& f) &&
    {
        this->event_initializer.init_on_action(static_cast<F&&>(f));
        return {this->event_initializer};
    }

    template<class F>
    CXX_WARN_UNUSED_RESULT NextSubExecutorImpl<F, Initial, i, 2/*0b010*/> on_timeout(F&& f) &&
    {
        this->event_initializer.init_on_timeout(static_cast<F&&>(f));
        return {this->event_initializer};
    }

    template<class F>
    CXX_WARN_UNUSED_RESULT NextSubExecutorImpl<F, Initial, i, 4/*0b100*/> on_exit(F&& f) &&
    {
        this->event_initializer.init_on_exit(static_cast<F&&>(f));
        return {this->event_initializer};
    }

    SubExecutorImpl(ExecutorEventInitializer event_initializer) noexcept
      : event_initializer(event_initializer)
    {}

private:
    ExecutorEventInitializer event_initializer;
};

#define MK_SubExecutorImpl(i, mem)                                           \
    template<>                                                               \
    struct SubExecutorImpl<i, false>                                         \
    {                                                                        \
        template<class F>                                                    \
        CXX_WARN_UNUSED_RESULT ExecutorResult mem(F&& f) &&                  \
        {                                                                    \
            this->event_initializer.init_##mem(static_cast<F&&>(f));         \
            return {ExecutorResult::SubExecutor, {}};                        \
        }                                                                    \
                                                                             \
        SubExecutorImpl(ExecutorEventInitializer event_initializer) noexcept \
        : event_initializer(event_initializer)                               \
        {}                                                                   \
                                                                             \
    private:                                                                 \
        ExecutorEventInitializer event_initializer;                          \
    };                                                                       \
                                                                             \
    template<>                                                               \
    struct SubExecutorImpl<i, true>                                          \
    {                                                                        \
        template<class F>                                                    \
        void mem(F&& f) &&                                                   \
        {                                                                    \
            this->event_initializer.init_##mem(static_cast<F&&>(f));         \
        }                                                                    \
                                                                             \
        SubExecutorImpl(ExecutorEventInitializer event_initializer) noexcept \
        : event_initializer(event_initializer)                               \
        {}                                                                   \
                                                                             \
    private:                                                                 \
        ExecutorEventInitializer event_initializer;                          \
    }

MK_SubExecutorImpl(6/*0b110*/, on_action);
MK_SubExecutorImpl(5/*0b101*/, on_timeout);
MK_SubExecutorImpl(3/*0b011*/, on_exit);

#undef MK_SubExecutorImpl

template<class F, bool Initial, int i, int flag>
using NextSubExecutorImpl = typename std::enable_if<
    ((void)static_cast<F*>(nullptr), !(i & flag)),
    SubExecutorImpl<i | flag, Initial>
>::type;


template<bool Initial>
struct SetSubExecutor
{
    template<class F>
    CXX_WARN_UNUSED_RESULT SubExecutorImpl<1/*0b001*/, Initial> on_action(F&& f) &&
    {
        return SubExecutorImpl<0, Initial>{this->event_initializer}.on_action(static_cast<F&&>(f));
    }

    template<class F>
    CXX_WARN_UNUSED_RESULT SubExecutorImpl<2/*0b010*/, Initial> on_timeout(F&& f) &&
    {
        return SubExecutorImpl<0, Initial>{this->event_initializer}.on_timeout(static_cast<F&&>(f));
    }

    template<class F>
    CXX_WARN_UNUSED_RESULT SubExecutorImpl<4/*0b100*/, Initial> on_exit(F&& f) &&
    {
        return SubExecutorImpl<0, Initial>{this->event_initializer}.on_exit(static_cast<F&&>(f));
    }

    explicit SetSubExecutor(ExecutorEventInitializer event_initializer) noexcept
      : event_initializer(event_initializer)
    {}

private:
    ExecutorEventInitializer event_initializer;
};

using InitialSubExecutor = SetSubExecutor<true>;
using SubExecutor = SetSubExecutor<false>;


struct Executor
{
    template<class... Args>
    CXX_WARN_UNUSED_RESULT InitialSubExecutor
    initial_executor(const char * /*TODO name*/, Args&&... args)
    {
        this->events.emplace_back(static_cast<Args&&>(args)...);
        return InitialSubExecutor{{this->events.back(), *this}};
    }

    bool exec();

private:
    CXX_WARN_UNUSED_RESULT SubExecutor sub_executor(const char * /*TODO name*/)
    {
        this->events.emplace_back();
        return SubExecutor{{this->events.back(), *this}};
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

    template<class F>
    CXX_WARN_UNUSED_RESULT SubExecutor sub_executor(const char * name, F&& f)
    {
        return this->executor.sub_executor(name, static_cast<F&&>(f));
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
        return {
            ExecutorResult::ReplaceTimeout,
            {static_cast<F&&>(f), nullptr, nullptr}
        };
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
        return {
            ExecutorResult::ReplaceAction,
            {static_cast<F&&>(f), nullptr, nullptr}
        };
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
            ExecutorResult r = this->events.back().on_exit(ExecutorActionContext{*this}, status);
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

    ExecutorResult r = this->events.back().on_action(ExecutorActionContext{*this});
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

template<class F>
void ExecutorEventInitializer::init_on_action(F&& f)
{
    this->executor_event.on_action = static_cast<F&&>(f);
}

template<class F>
void ExecutorEventInitializer::init_on_exit(F&& f)
{
    this->executor_event.on_exit = static_cast<F&&>(f);
}

template<class F>
void ExecutorEventInitializer::init_on_timeout(F&& f)
{
    this->executor_event.on_timeout = static_cast<F&&>(f);
}
