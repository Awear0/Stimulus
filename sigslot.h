#ifndef SIGSLOT_H_
#define SIGSLOT_H_

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

template<class NotVoid>
concept not_void = (!std::same_as<NotVoid, void>);

template<class SignalArgs>
concept signal_arg = (not_void<SignalArgs> && !std::is_rvalue_reference_v<SignalArgs>);

class connection;

class emitter
{
public:
    friend class connection;

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

private:
    class connection_holder;
};

class emitter::connection_holder
{
public:
    virtual ~connection_holder() = default;

    virtual void disconnect() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
};

class connection
{
public:
    connection(std::weak_ptr<emitter::connection_holder> weak_holder):
        m_holder { std::move(weak_holder) }
    {
    }

    connection(const connection&) = default;
    connection(connection&&) = default;
    auto operator=(const connection&) -> connection& = default;
    auto operator=(connection&&) -> connection& = default;

    void disconnect()
    {
        auto locked_holder { m_holder.lock() };
        if (!locked_holder)
        {
            return;
        }

        locked_holder->disconnect();
    }

    void suspend()
    {
        auto locked_holder { m_holder.lock() };
        if (!locked_holder)
        {
            return;
        }

        locked_holder->suspend();
    }

    void resume()
    {
        auto locked_holder { m_holder.lock() };
        if (!locked_holder)
        {
            return;
        }

        locked_holder->resume();
    }

private:
    std::weak_ptr<emitter::connection_holder> m_holder;
};

class scoped_connection
{
public:
    scoped_connection(connection conn):
        m_connection { std::move(conn) }
    {
    }

    ~scoped_connection()
    {
        disconnect();
    }

    void disconnect()
    {
        m_connection.disconnect();
    }

private:
    connection m_connection;
};

class inhibitor
{
public:
    inhibitor(connection conn):
        m_connection { std::move(conn) }
    {
        m_connection.suspend();
    }

    ~inhibitor()
    {
        m_connection.resume();
    }

private:
    connection m_connection;
};

template<std::size_t Index, class... Args>
concept in_args_range = (Index < sizeof...(Args));

template<std::size_t... Values>
struct all_different_implementation: std::true_type
{
};

template<std::size_t... Values>
constexpr bool all_different_implementation_t { all_different_implementation<Values...>::value };

template<std::size_t Value>
struct all_different_implementation<Value>: std::true_type
{
};

template<std::size_t Value, std::size_t... Tail>
struct all_different_implementation<Value, Tail...>
    : std::conditional_t<((Value == Tail) || ...), std::false_type, std::true_type>
{
};

template<std::size_t... Values>
concept all_different = all_different_implementation_t<Values...>;

// TODO AROSS: operator= strategy for signal
template<signal_arg... Args>
class emitter::signal final
{
public:
    friend emitter;
    using slot = std::function<void(Args...)>;

    template<std::invocable<Args...> Callable>
    auto connect(Callable&& callable) const -> connection;

    template<std::invocable<Args...> Callable>
    auto connect_once(Callable&& callable) const -> connection;

    template<std::size_t... Indexes>
        requires((in_args_range<Indexes, Args...> && ...) && all_different<Indexes...>)
    class mapped_signal;

    template<std::size_t... Indexes>
        requires((in_args_range<Indexes, Args...> && ...) && all_different<Indexes...>)
    auto map() -> mapped_signal<Indexes...>;

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

        // TODO AROSS: Handle exceptions

        for (auto it { begin }; it != previous_to_end; ++it)
        {
            (**it)(emitted_args...);
        }

        (*m_slots.back())(std::forward<EmittedArgs>(emitted_args)...);
    }

    class connection_holder_implementation;

    void disconnect(connection_holder_implementation* holder) const
    {
        std::erase_if(m_slots, [holder](const auto& slot) { return slot.get() == holder; });
    }

    mutable std::vector<std::shared_ptr<connection_holder_implementation>> m_slots {};
};

template<signal_arg... Args>
template<std::size_t... Indexes>
    requires((in_args_range<Indexes, Args...> && ...) && all_different<Indexes...>)
class emitter::signal<Args...>::mapped_signal
{
public:
    mapped_signal(const signal& emitted_signal):
        m_signal { emitted_signal }
    {
    }

    template<std::invocable<Args...[Indexes]...> Callable>
    auto connect(Callable&& callable) const&& -> connection
    {
        return m_signal.connect([callable = std::forward<Callable>(callable)](Args&&... args)
        { callable(std::forward<Args...[Indexes]>(args...[Indexes])...); });
    }

    template<std::invocable<Args...[Indexes]...> Callable>
    auto connect_once(Callable&& callable) const&& -> connection
    {
        return m_signal.connect_once([callable = std::forward<Callable>(callable)](Args&&... args)
        { callable(std::forward<Args...[Indexes]>(args...[Indexes])...); });
    }

private:
    const signal& m_signal;
};

template<signal_arg... Args>
template<std::size_t... Indexes>
    requires((in_args_range<Indexes, Args...> && ...) && all_different<Indexes...>)
auto emitter::signal<Args...>::map() -> mapped_signal<Indexes...>
{
    return { *this };
}

template<signal_arg... Args>
class emitter::signal<Args...>::connection_holder_implementation final: public connection_holder

{
public:
    template<std::convertible_to<signal::slot> Callable>
    connection_holder_implementation(const signal& connected_signal,
                                     Callable&& callable,
                                     bool single_shot = false):
        m_signal { connected_signal },
        m_slot { std::forward<Callable>(callable) },
        m_single_shot { single_shot }
    {
    }

    template<class... EmittedArgs>
        requires std::invocable<signal::slot, EmittedArgs...>
    void operator()(EmittedArgs&&... args)
    {
        if (m_suspended)
        {
            return;
        }
        if (m_single_shot)
        {
            disconnect();
        }
        m_slot(std::forward<EmittedArgs>(args)...);
    }

    void disconnect() override
    {
        m_signal.disconnect(this);
    }

    void suspend() override
    {
        m_suspended = true;
    }

    void resume() override
    {
        m_suspended = false;
    }

private:
    signal::slot m_slot;
    const signal& m_signal;
    bool m_suspended { false };
    bool m_single_shot;
};

template<signal_arg... Args>
template<std::invocable<Args...> Callable>
auto emitter::signal<Args...>::connect(Callable&& callable) const -> connection
{
    m_slots.emplace_back(
        std::make_shared<connection_holder_implementation>(*this,
                                                           std::forward<Callable>(callable)));

    return { m_slots.back() };
}

template<signal_arg... Args>
template<std::invocable<Args...> Callable>
auto emitter::signal<Args...>::connect_once(Callable&& callable) const -> connection
{
    m_slots.emplace_back(
        std::make_shared<connection_holder_implementation>(*this,
                                                           std::forward<Callable>(callable),
                                                           true));

    return { m_slots.back() };
}

#endif // SIGSLOT_H_