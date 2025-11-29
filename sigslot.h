#ifndef SIGSLOT_H_
#define SIGSLOT_H_

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

template<class NotVoid>
concept not_void = (!std::same_as<NotVoid, void>);

template<class SignalArgs>
concept signal_arg = (not_void<SignalArgs> && !std::is_rvalue_reference_v<SignalArgs>);

class emitter
{
public:
    virtual ~emitter() = default;

protected:
    template<signal_arg... Args>
    class signal;

    template<class Emitter, signal_arg... Args, std::convertible_to<Args>... EmittedArgs>
    void emit(this const Emitter& self,
              signal<Args...> Emitter::* emitted_signal,
              EmittedArgs&&... emitted_args)
    {
        (self.*emitted_signal).emit(std::forward<EmittedArgs>(emitted_args)...);
    }
};

template<signal_arg... Args>
class emitter::signal final
{
public:
    friend emitter;
    using slot = std::function<void(Args...)>;

    template<std::invocable<Args...> Callable>
    void connect(Callable&& callable) const
    {
        m_slots.emplace_back(std::forward<Callable>(callable));
    }

private:
    template<std::convertible_to<Args>... EmittedArgs>
    void emit(EmittedArgs&&... emitted_args) const
    {
        if (m_slots.empty())
        {
            return;
        }

        auto begin { m_slots.begin() };
        auto previous_to_end { std::prev(m_slots.end()) };

        for (auto it { begin }; it != previous_to_end; ++it)
        {
            (*it)(emitted_args...);
        }

        m_slots.back()(std::forward<EmittedArgs>(emitted_args)...);
    }

    mutable std::vector<slot> m_slots {};
};

#endif // SIGSLOT_H_