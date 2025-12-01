#ifndef SIGSLOT_H_
#define SIGSLOT_H_

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// ### Helpers

template<class ExecutionPolicy>
concept execution_policy = requires(ExecutionPolicy policy) {
    policy.execute(std::function<void()> {});
    { std::remove_cvref_t<ExecutionPolicy>::is_synchronous } -> std::convertible_to<bool>;
};

struct synchronous_policy
{
    template<std::invocable Invocable>
    void execute(Invocable&& invocable)
    {
        invocable();
    }

    static constexpr bool is_synchronous { true };
};

class execution_policy_holder_implementation_interface
{
public:
    virtual ~execution_policy_holder_implementation_interface() = default;

    virtual void execute(std::function<void()> invocable) = 0;
    virtual auto is_synchronous() const -> bool = 0;
};

template<execution_policy Policy>
class execution_policy_holder_implementation
    : public execution_policy_holder_implementation_interface
{
public:
    execution_policy_holder_implementation(Policy&& policy):
        m_policy { std::forward<Policy>(policy) }
    {
    }

    void execute(std::function<void()> invocable) override
    {
        m_policy.execute(std::move(invocable));
    }

    auto is_synchronous() const -> bool override
    {
        return std::remove_cvref_t<Policy>::is_synchronous;
    }

private:
    Policy m_policy;
};

class execution_policy_holder
{
public:
    template<execution_policy Policy>
    execution_policy_holder(Policy&& policy):
        m_policy_holder { std::make_unique<execution_policy_holder_implementation<Policy>>(
            std::forward<Policy>(policy)) }
    {
    }

    template<std::invocable Invocable>
    void execute(Invocable&& invocable)
    {
        m_policy_holder->execute(std::forward<Invocable>(invocable));
    }

    auto is_synchronous() const -> bool
    {
        return m_policy_holder->is_synchronous();
    }

private:
    std::unique_ptr<execution_policy_holder_implementation_interface> m_policy_holder;
};

template<class NotVoid>
concept not_void = (!std::same_as<NotVoid, void>);

template<class SignalArgs>
concept signal_arg = (not_void<SignalArgs> && !std::is_rvalue_reference_v<SignalArgs>);

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
    : std::conditional_t<((Value == Tail) || ...) || !all_different_implementation_t<Tail...>,
                         std::false_type,
                         all_different_implementation<Tail...>>
{
};

template<std::size_t... Values>
concept all_different = all_different_implementation_t<Values...>;

// ### Forward declaration

class connection;

template<class SequenceIndex, signal_arg... Args>
class mapped_signal;

template<class MappedSignal>
concept is_mapped_signal = requires(const MappedSignal& mapped_sig) {
    []<std::size_t... Indexes, signal_arg... Args>(
        const mapped_signal<std::index_sequence<Indexes...>, Args...>&) {}(mapped_sig);
};

template<class Signal, class... Transformations>
class transformed_signal;

template<class TransformedSignal>
concept is_transformed_signal = requires(const TransformedSignal& transformed_signal) {
    []<class Signal, signal_arg... Args>(const ::transformed_signal<Signal, Args...>&) {}(
        transformed_signal);
};

template<class Signal, class IndexSequence, class... Transformations>
class mapped_transformed_signal;

template<class MappedTransformedSignal>
concept is_mapped_transformed_signal = requires(
    const MappedTransformedSignal& mapped_transformed_signal) {
    []<class Signal, std::size_t... Indexes, signal_arg... Args>(
        const ::mapped_transformed_signal<Signal, std::index_sequence<Indexes...>, Args...>&) {}(
        mapped_transformed_signal);
};

template<class SignalTransformation>
concept signal_transformation =
    (is_mapped_signal<SignalTransformation> || is_transformed_signal<SignalTransformation> ||
     is_mapped_transformed_signal<SignalTransformation>);

// ### Class emitter declaration

class emitter
{
public:
    friend class connection;

    virtual ~emitter() = default;

protected:
    template<signal_arg... Args>
    class signal;

    template<class Signal>
    struct is_signal: public std::false_type
    {
    };

    template<signal_arg... Args>
    struct is_signal<signal<Args...>>: std::true_type
    {
    };

    template<class Signal>
    static constexpr bool is_signal_v { is_signal<Signal>::value };

    template<class SequenceIndex, signal_arg... Args>
    friend class mapped_signal;

    template<class Signal, class... Transformations>
    friend class transformed_signal;

    template<class Signal, class IndexSequence, class... Transformations>
    friend class mapped_transformed_signal;

    template<class Emitter, signal_arg... Args, std::convertible_to<Args>... EmittedArgs>
    void emit(this const Emitter& self,
              signal<Args...> Emitter::* emitted_signal,
              EmittedArgs&&... emitted_args)
    {
        (self.*emitted_signal).emit(std::forward<EmittedArgs>(emitted_args)...);
    }

protected:
    // TODO AROSS: Handle automatic removal when receiver is destroyed
    template<class Receiver, signal_arg... ReceiverArgs, class Emitter>
        requires(signal_transformation<std::remove_cvref_t<Emitter>> ||
                 is_signal_v<std::remove_cvref_t<Emitter>>)
    auto connect(this const Receiver& self,
                 Emitter&& emitter,
                 signal<ReceiverArgs...> Receiver::* receiver_signal) -> connection;

    template<class Receiver, signal_arg... ReceiverArgs, class Emitter>
        requires(signal_transformation<std::remove_cvref_t<Emitter>> ||
                 is_signal_v<std::remove_cvref_t<Emitter>>)
    auto connect_once(this const Receiver& self,
                      Emitter&& emitter,
                      signal<ReceiverArgs...> Receiver::* receiver_signal) -> connection;

private:
    template<class Receiver, signal_arg... ReceiverArgs>
    auto forwarding_lambda(this const Receiver& self,
                           signal<ReceiverArgs...> Receiver::* receiver_signal);

    class connection_holder;
};

// ### Connection related classes

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

// ### Signal definition

// TODO AROSS: operator= strategy for signal
template<signal_arg... Args>
class emitter::signal final
{
public:
    friend emitter;
    using slot = std::function<void(Args...)>;

    template<std::invocable<Args...> Callable, execution_policy Policy = synchronous_policy>
    auto connect(Callable&& callable, Policy&& policy = {}) const -> connection;

    template<std::invocable<Args...> Callable, execution_policy Policy = synchronous_policy>
    auto connect_once(Callable&& callable, Policy&& policy = {}) const -> connection;

    template<std::size_t... Indexes>
        requires((in_args_range<Indexes, Args...> && ...) && all_different<Indexes...>)
    auto map() const -> mapped_signal<std::index_sequence<Indexes...>, Args...>;

private:
    template<std::size_t Value>
    using identity_t = const std::identity&;

    template<class IndexSequence, class... Transformations>
    struct fill_transformed_signal;

    template<std::size_t... Indexes, class... Transformations>
    struct fill_transformed_signal<std::index_sequence<Indexes...>, Transformations...>
    {
        using type = transformed_signal<signal, Transformations..., identity_t<Indexes>...>;
    };

    template<class... Transformations>
    using fill_transformed_signal_t = fill_transformed_signal<
        std::make_index_sequence<sizeof...(Args) - sizeof...(Transformations)>,
        Transformations...>::type;

public:
    template<std::invocable<Args>... Transformations>
        requires((sizeof...(Transformations) == sizeof...(Args)) &&
                 (not_void<std::invoke_result_t<Transformations, Args>> && ...))
    auto transform(Transformations&&... transformations) const
        -> transformed_signal<signal, Transformations...>;

    template<class... Transformations>
        requires(sizeof...(Transformations) < sizeof...(Args))
    auto transform(Transformations&&... transformations) const
        -> fill_transformed_signal_t<Transformations...>;

private:
    template<std::convertible_to<Args>... EmittedArgs>
    void emit(EmittedArgs&&... emitted_args) const
    {
        if (m_slots.empty())
        {
            return;
        }

        auto slots { m_slots };

        auto begin { slots.begin() };
        auto previous_to_end { std::prev(slots.end()) };

        // TODO AROSS: Handle exceptions

        for (auto it { begin }; it != previous_to_end; ++it)
        {
            (**it)(emitted_args...);
        }

        (*slots.back())(std::forward<EmittedArgs>(emitted_args)...);
    }

    class connection_holder_implementation;

    void disconnect(connection_holder_implementation* holder) const
    {
        std::erase_if(m_slots, [holder](const auto& slot) { return slot.get() == holder; });
    }

    template<std::size_t... IdentityIndexes, class... Transformations>
    auto partial_transformation(const std::index_sequence<IdentityIndexes...>&,
                                Transformations&&... transformations) const
        -> fill_transformed_signal_t<Transformations...>;

    mutable std::vector<std::shared_ptr<connection_holder_implementation>> m_slots {};
};

// ### mapped_signal definition

template<std::size_t... Indexes, class... Args>
    requires((in_args_range<Indexes, Args...> && ...) && all_different<Indexes...>)
class mapped_signal<std::index_sequence<Indexes...>, Args...>
{
public:
    mapped_signal(const emitter::signal<Args...>& emitted_signal):
        m_signal { emitted_signal }
    {
    }

    template<std::invocable<Args...[Indexes]...> Callable>
    auto connect(Callable&& callable) const&& -> connection
    {
        return m_signal.connect(forwarding_lambda(std::forward<Callable>(callable)));
    }

    template<std::invocable<Args...[Indexes]...> Callable>
    auto connect_once(Callable&& callable) const&& -> connection
    {
        return m_signal.connect_once(forwarding_lambda(std::forward<Callable>(callable)));
    }

    template<std::invocable<Args...[Indexes]>... Transformations>
        requires((sizeof...(Transformations) == sizeof...(Args)) &&
                 (not_void<std::invoke_result_t<Transformations, Args...[Indexes]>> && ...))
    auto transform(Transformations&&... transformations)
        const&& -> mapped_transformed_signal<emitter::signal<Args...>,
                                             std::index_sequence<Indexes...>,
                                             Transformations...>
    {
        return { m_signal, std::forward<Transformations>(transformations)... };
    }

    template<class... Transformations>
        requires(sizeof...(Transformations) < sizeof...(Args))
    auto transform(Transformations&&... transformations) const&&
    {
        return std::move(*this).partial_transformation(
            std::make_index_sequence<sizeof...(Args) - sizeof...(Transformations)> {},
            std::forward<Transformations>(transformations)...);
    }

private:
    template<std::size_t... IdentityIndexes, class... Transformations>
    auto partial_transformation(const std::index_sequence<IdentityIndexes...>&,
                                Transformations&&... transformations) const
    {
        static constexpr std::identity identity {};

        static constexpr auto create_identity {
            [](std::size_t Index) constexpr -> const std::identity& { return identity; }
        };

        return std::move(*this).transform(std::forward<Transformations>(transformations)...,
                                          create_identity(IdentityIndexes)...);
    }

    template<std::invocable<Args...[Indexes]...> Callable>
    auto forwarding_lambda(Callable&& callable) const
    {
        return [callable = std::forward<Callable>(callable)](Args&&... args) mutable
        { callable(std::forward<Args...[Indexes]>(args...[Indexes])...); };
    }

    const emitter::signal<Args...>& m_signal;
};

// ### transformed_signal class

template<signal_arg... Args, class... Transformations>
    requires((std::invocable<Transformations, Args> && ...) &&
             (not_void<std::invoke_result_t<Transformations, Args>> && ...))
class transformed_signal<emitter::signal<Args...>, Transformations...>
{
public:
    friend emitter;

    transformed_signal(const emitter::signal<Args...>& emitted_signal,
                       Transformations&&... transformations):
        m_signal { emitted_signal },
        m_transformations { std::forward<Transformations>(transformations)... }
    {
    }

    template<std::invocable<std::invoke_result_t<Transformations, Args>...> Callable>
    auto connect(Callable&& callable) && -> connection
    {
        return m_signal.connect(forwarding_lambda(std::forward<Callable>(callable)));
    }

    template<std::invocable<std::invoke_result_t<Transformations, Args>...> Callable>
    auto connect_once(Callable&& callable) && -> connection
    {
        return m_signal.connect_once(forwarding_lambda(std::forward<Callable>(callable)));
    }

private:
    template<std::invocable<std::invoke_result_t<Transformations, Args>...> Callable>
    auto forwarding_lambda(Callable&& callable)
    {
        return [&callable, this]<std::size_t... Indexes>(const std::index_sequence<Indexes...>&)
        {
            return [callable = std::forward<Callable>(callable),
                    transformations = std::move(m_transformations)](Args&&... args) mutable
            { callable(std::get<Indexes>(transformations)(std::forward<Args>(args))...); };
        }(std::make_index_sequence<sizeof...(Transformations)> {});
    }

    const emitter::signal<Args...>& m_signal;
    std::tuple<Transformations...> m_transformations;
};

// ### mapped_transformed_signal class

template<signal_arg... Args, std::size_t... Indexes, class... Transformations>
    requires(((in_args_range<Indexes, Args...> && ...) && all_different<Indexes...>) &&
             ((std::invocable<Transformations, Args...[Indexes]> && ...) &&
              (not_void<std::invoke_result_t<Transformations, Args...[Indexes]>> && ...)))
class mapped_transformed_signal<emitter::signal<Args...>,
                                std::index_sequence<Indexes...>,
                                Transformations...>
{
public:
    mapped_transformed_signal(const emitter::signal<Args...>& emitted_signal,
                              Transformations... transformations):
        m_signal { emitted_signal },
        m_transformations { std::forward<Transformations>(transformations)... }
    {
    }

    template<std::invocable<std::invoke_result_t<Transformations, Args...[Indexes]>...> Callable>
    auto connect(Callable&& callable) && -> connection
    {
        return m_signal.connect(forwarding_lambda(std::forward<Callable>(callable)));
    }

    template<std::invocable<std::invoke_result_t<Transformations, Args...[Indexes]>...> Callable>
    auto connect_once(Callable&& callable) && -> connection
    {
        return m_signal.connect_once(forwarding_lambda(std::forward<Callable>(callable)));
    }

private:
    template<std::invocable<std::invoke_result_t<Transformations, Args...[Indexes]>...> Callable>
    auto forwarding_lambda(Callable&& callable)
    {
        return
            [callable = std::forward<Callable>(callable),
             transformations = std::move(m_transformations)]<std::size_t... TransformationIndexes>(
                const std::index_sequence<TransformationIndexes...>&) mutable
        {
            return [callable = std::move(callable),
                    transformations = std::move(transformations)](Args&&... args) mutable
            {
                callable(std::get<TransformationIndexes>(transformations)(
                    std::forward<Args...[Indexes]>(args...[Indexes]))...);
            };
        }(std::make_index_sequence<sizeof...(Transformations)> {});
    }

    const emitter::signal<Args...>& m_signal;
    std::tuple<Transformations...> m_transformations;
};

// ### connection_holder_implementation class

template<signal_arg... Args>
class emitter::signal<Args...>::connection_holder_implementation final: public connection_holder
{
    template<class T>
    using ref_or_value = std::conditional_t<std::is_lvalue_reference_v<T>,
                                            std::reference_wrapper<std::remove_reference_t<T>>,
                                            T>;

public:
    template<std::convertible_to<signal::slot> Callable, execution_policy Policy>
    connection_holder_implementation(const signal& connected_signal,
                                     Callable&& callable,
                                     Policy&& policy,
                                     bool single_shot = false):
        m_slot { std::forward<Callable>(callable) },
        m_signal { connected_signal },
        m_policy(std::forward<Policy>(policy)),
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

        if (m_policy.is_synchronous())
        {
            m_policy.execute([&] { m_slot(std::forward<EmittedArgs>(args)...); });
        }
        else
        {
            m_policy.execute(
                [slot = m_slot,
                 ... args = ref_or_value<Args>(std::forward<EmittedArgs>(args))] mutable
            { slot(std::forward<Args>(args)...); });
        }
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
    execution_policy_holder m_policy;
    bool m_suspended { false };
    bool m_single_shot;
};

// ### emitter implementation

template<class Receiver, signal_arg... ReceiverArgs, class Emitter>
    requires(signal_transformation<std::remove_cvref_t<Emitter>> ||
             emitter::is_signal_v<std::remove_cvref_t<Emitter>>)
auto emitter::connect(this const Receiver& self,
                      Emitter&& emitter,
                      signal<ReceiverArgs...> Receiver::* receiver_signal) -> connection
{
    if constexpr (signal_transformation<std::remove_cvref_t<Emitter>>)
    {
        return std::forward<Emitter>(emitter).connect(self.forwarding_lambda(receiver_signal));
    }
    else
    {
        return emitter.connect(self.forwarding_lambda(receiver_signal));
    }
}

template<class Receiver, signal_arg... ReceiverArgs, class Emitter>
    requires(signal_transformation<std::remove_cvref_t<Emitter>> ||
             emitter::is_signal_v<std::remove_cvref_t<Emitter>>)
auto emitter::connect_once(this const Receiver& self,
                           Emitter&& emitter,
                           signal<ReceiverArgs...> Receiver::* receiver_signal) -> connection
{
    if constexpr (signal_transformation<std::remove_cvref_t<Emitter>>)
    {
        return std::forward<Emitter>(emitter).connect_once(self.forwarding_lambda(receiver_signal));
    }
    else
    {
        return emitter.connect_once(self.forwarding_lambda(receiver_signal));
    }
}

template<class Receiver, signal_arg... ReceiverArgs>
auto emitter::forwarding_lambda(this const Receiver& self,
                                signal<ReceiverArgs...> Receiver::* receiver_signal)
{
    return [&emitted_signal = self.*receiver_signal]<signal_arg... Args>(Args&&... args)
    { emitted_signal.emit(std::forward<Args>(args)...); };
}

template<signal_arg... Args>
template<std::size_t... Indexes>
    requires((in_args_range<Indexes, Args...> && ...) && all_different<Indexes...>)
auto emitter::signal<Args...>::map() const
    -> mapped_signal<std::index_sequence<Indexes...>, Args...>
{
    return { *this };
}

// ### signal implementation

template<signal_arg... Args>
template<std::invocable<Args>... Transformations>
    requires((sizeof...(Transformations) == sizeof...(Args)) &&
             (not_void<std::invoke_result_t<Transformations, Args>> && ...))
auto emitter::signal<Args...>::transform(Transformations&&... transformations) const
    -> transformed_signal<signal<Args...>, Transformations...>
{
    return { *this, std::forward<Transformations>(transformations)... };
}

template<signal_arg... Args>
template<class... Transformations>
    requires(sizeof...(Transformations) < sizeof...(Args))
auto emitter::signal<Args...>::transform(Transformations&&... transformations) const
    -> fill_transformed_signal_t<Transformations...>
{
    return std::move(*this).partial_transformation(
        std::make_index_sequence<sizeof...(Args) - sizeof...(Transformations)> {},
        std::forward<Transformations>(transformations)...);
}

template<signal_arg... Args>
template<std::size_t... IdentityIndexes, class... Transformations>
auto emitter::signal<Args...>::partial_transformation(
    const std::index_sequence<IdentityIndexes...>&,
    Transformations&&... transformations) const -> fill_transformed_signal_t<Transformations...>
{
    static constexpr std::identity identity {};

    static constexpr auto create_identity { [](std::size_t Index) constexpr -> const std::identity&
    { return identity; } };

    return std::move(*this).transform(std::forward<Transformations>(transformations)...,
                                      create_identity(IdentityIndexes)...);
}

template<signal_arg... Args>
template<std::invocable<Args...> Callable, execution_policy Policy>
auto emitter::signal<Args...>::connect(Callable&& callable, Policy&& policy) const -> connection
{
    m_slots.emplace_back(
        std::make_shared<connection_holder_implementation>(*this,
                                                           std::forward<Callable>(callable),
                                                           std::forward<Policy>(policy)));

    return { m_slots.back() };
}

template<signal_arg... Args>
template<std::invocable<Args...> Callable, execution_policy Policy>
auto emitter::signal<Args...>::connect_once(Callable&& callable, Policy&& policy) const
    -> connection
{
    m_slots.emplace_back(
        std::make_shared<connection_holder_implementation>(*this,
                                                           std::forward<Callable>(callable),
                                                           std::forward<Policy>(policy),
                                                           true));

    return { m_slots.back() };
}

#endif // SIGSLOT_H_