#ifndef SIGSLOT_H_
#define SIGSLOT_H_

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// ### Helpers

template<class Instance, template<class...> class Template>
concept instance_of = requires(const Instance& instance) {
    []<class... Args>(const Template<Args...>&) {}(instance);
};

template<class Tuple>
concept is_tuple = instance_of<Tuple, std::tuple>;

template<class ExecutionPolicy>
concept execution_policy = requires(ExecutionPolicy policy) {
    policy.execute(std::function<void()> {});
    { std::remove_cvref_t<ExecutionPolicy>::is_synchronous } -> std::convertible_to<bool>;
};

struct synchronous_policy
{
    template<std::invocable Invocable>
    void execute(Invocable&& invocable) const
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
    template<std::invocable Callable>
    struct policy_visitor_executor
    {
        policy_visitor_executor(Callable&& callable):
            m_callable { std::forward<Callable>(callable) }
        {
        }

        void operator()(const synchronous_policy& policy)
        {
            policy.execute(std::forward<Callable>(m_callable));
        }

        void operator()(
            const std::unique_ptr<execution_policy_holder_implementation_interface>& policy)
        {
            policy->execute(std::forward<Callable>(m_callable));
        }

        Callable&& m_callable;
    };

    struct policy_visitor_synchronicity_checker
    {
        static auto constexpr operator()(const synchronous_policy& policy) -> bool
        {
            return synchronous_policy::is_synchronous;
        }

        static auto operator()(
            const std::unique_ptr<execution_policy_holder_implementation_interface>& policy) -> bool
        {
            return policy->is_synchronous();
        }
    };

    static constexpr policy_visitor_synchronicity_checker synchronicity_checker {};

public:
    execution_policy_holder(const synchronous_policy& policy):
        m_policy { policy }
    {
    }

    execution_policy_holder(synchronous_policy&& policy):
        m_policy { std::move(policy) }
    {
    }

    template<execution_policy Policy>
    execution_policy_holder(Policy&& policy):
        m_policy { std::make_unique<execution_policy_holder_implementation<Policy>>(
            std::forward<Policy>(policy)) }
    {
    }

    template<std::invocable Invocable>
    void execute(Invocable&& invocable)
    {
        std::visit(policy_visitor_executor { std::forward<Invocable>(invocable) }, m_policy);
    }

    auto is_synchronous() const -> bool
    {
        return std::visit(synchronicity_checker, m_policy);
    }

private:
    std::variant<synchronous_policy,
                 std::unique_ptr<execution_policy_holder_implementation_interface>>
        m_policy;
};

template<class NotVoid>
concept not_void = (!std::same_as<NotVoid, void>);

template<class SignalArgs>
concept signal_arg = (not_void<SignalArgs> && !std::is_rvalue_reference_v<SignalArgs>);

template<std::size_t Index, class... Args>
concept in_args_range = (Index < sizeof...(Args));

template<std::size_t Index, class Tuple>
concept in_tuple_range = requires {
    is_tuple<Tuple>;
    Index < std::tuple_size_v<Tuple>;
};

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
    : std::conditional_t<((Value == Tail) || ...),
                         std::false_type,
                         all_different_implementation<Tail...>>
{
};

template<std::size_t... Values>
concept all_different = all_different_implementation_t<Values...>;

// ### Partial call

template<class Callable, class... Args>
struct is_partially_callable;

template<class Callable, class... Args>
constexpr bool is_partially_callable_v { is_partially_callable<Callable, Args...>::value };

template<class Callable, std::size_t Count, class... Args>
struct is_partially_callable_with_argument_count;

template<class Callable, class... Args>
struct is_partially_callable_with_argument_count<Callable, 0, Args...>: std::is_invocable<Callable>
{
};

template<class Callable, class IndexSequence, class... Args>
struct is_partially_callable_with_argument_count_impl;

template<class Callable, std::size_t... Indexes, class... Args>
struct is_partially_callable_with_argument_count_impl<Callable,
                                                      std::index_sequence<Indexes...>,
                                                      Args...>
    : std::is_invocable<Callable, Args...[Indexes]...>
{
};

template<class Callable, std::size_t Count, class... Args>
struct is_partially_callable_with_argument_count
    : std::conditional_t<
          is_partially_callable_with_argument_count_impl<Callable,
                                                         std::make_index_sequence<Count>,
                                                         Args...>::value,
          std::true_type,
          is_partially_callable_with_argument_count<Callable, Count - 1, Args...>>
{
};

template<class Callable, class... Args>
struct is_partially_callable
    : is_partially_callable_with_argument_count<Callable, sizeof...(Args), Args...>
{
};

template<class Callable, class... Args>
concept partially_callable = is_partially_callable_v<Callable, Args...>;

template<class Callable, class... Args, std::size_t... Indexes>
    requires partially_callable<Callable, Args...>
constexpr auto
    partial_call_impl(Callable&& callable, std::index_sequence<Indexes...>, Args&&... args)
{
    if constexpr (std::invocable<Callable, Args...[Indexes]...>)
    {
        return std::invoke(std::forward<Callable>(callable),
                           std::forward<Args...[Indexes]>(args...[Indexes])...);
    }
    else
    {
        constexpr std::make_index_sequence<sizeof...(Indexes) - 1> index_sequence {};
        return partial_call_impl(std::forward<Callable>(callable),
                                 index_sequence,
                                 std::forward<Args>(args)...);
    }
}

template<class Callable, class... Args>
    requires partially_callable<Callable, Args...>
constexpr auto partial_call(Callable&& callable, Args&&... args)
{
    constexpr std::make_index_sequence<sizeof...(Args)> index_sequence {};
    return partial_call_impl(std::forward<Callable>(callable),
                             index_sequence,
                             std::forward<Args>(args)...);
}

template<class Callable, class Tuple>
concept partially_tuple_callable = requires(Callable callable, Tuple tuple) {
    is_tuple<Tuple>;
    {
        [callable, tuple]<std::size_t... Indexes>(const std::index_sequence<Indexes...>&)
        {
            partial_call(
                callable,
                std::forward<std::tuple_element_t<Indexes, Tuple>>(std::get<Indexes>(tuple))...);
        }(std::make_index_sequence<std::tuple_size_v<Tuple>> {})
    };
};

template<class Callable, class Tuple>
    requires partially_tuple_callable<Callable, Tuple>
constexpr auto partial_tuple_call(Callable&& callable, Tuple& tuple)
{
    constexpr std::make_index_sequence<std::tuple_size_v<Tuple>> index_sequence {};
    return []<std::size_t... Indexes>(const std::index_sequence<Indexes...>&,
                                      Callable&& callable,
                                      Tuple& tuple)
    {
        return partial_call(std::forward<Callable>(callable), std::get<Indexes>(tuple)...);
    }(index_sequence, std::forward<Callable>(callable), tuple);
}

// ### Forward declaration

class connection;
class source;

template<class Source>
concept source_like =
    requires(const Source& source, typename std::remove_cvref_t<Source>::args tuple) {
        std::derived_from<std::remove_cvref_t<Source>, ::source>;
        is_tuple<typename std::remove_cvref_t<Source>::args>;
        {
            []<class... Args>(const std::tuple<Args...>&)
            { source.connect(std::function<void(Args...)> {}); }(tuple)
        };
        {
            []<class... Args>(const std::tuple<Args...>&)
            { source.connect_once(std::function<void(Args...)> {}); }(tuple)
        };
    };

template<class Appliable, class Source>
concept appliable = requires(const Source& source, Appliable appliable) {
    source_like<Source>;
    { appliable.accept(source) } -> source_like;
};

// ### Class source
class source
{
public:
    template<source_like Self, appliable<Self> Appliable>
    auto apply(this const Self& self, Appliable&& appliable)
    {
        return appliable.accept(self);
    }
};

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
    template<class Receiver,
             signal_arg... ReceiverArgs,
             source_like Emitter,
             execution_policy Policy = synchronous_policy>
        requires(partially_tuple_callable<std::function<void(ReceiverArgs...)>,
                                          typename std::remove_cvref_t<Emitter>::args>)
    auto connect(this const Receiver& self,
                 Emitter&& emitter,
                 signal<ReceiverArgs...> Receiver::* receiver_signal,
                 Policy&& policy = {}) -> connection;

    template<class Receiver,
             signal_arg... ReceiverArgs,
             source_like Emitter,
             execution_policy Policy = synchronous_policy>
        requires(partially_tuple_callable<std::function<void(ReceiverArgs...)>,
                                          typename std::remove_cvref_t<Emitter>::args>)
    auto connect_once(this const Receiver& self,
                      Emitter&& emitter,
                      signal<ReceiverArgs...> Receiver::* receiver_signal,
                      Policy&& policy = {}) -> connection;

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

// ### connectable

// TODO AROSS:
class connectable2
{
public:
    template<class Self, class Callable, execution_policy Policy = synchronous_policy>
    auto connect(this Self&& self, Callable&& callable, Policy&& policy = {}) -> connection
    {
        return self.m_source.connect(self.forwarding_lambda(std::forward<Callable>(callable)),
                                     std::forward<Policy>(policy));
    }

    template<class Self, class Callable, execution_policy Policy = synchronous_policy>
    auto connect_once(this Self&& self, Callable&& callable, Policy&& policy = {}) -> connection
    {
        return self.m_source.connect_once(self.forwarding_lambda(std::forward<Callable>(callable)),
                                          std::forward<Policy>(policy));
    }
};

// ### Signal definition

// TODO AROSS: operator= strategy for signal
template<signal_arg... Args>
class emitter::signal final: public source
{
public:
    using args = std::tuple<Args...>;

    friend emitter;
    using slot = std::function<void(Args...)>;

    template<partially_callable<Args...> Callable, execution_policy Policy = synchronous_policy>
    auto connect(Callable&& callable, Policy&& policy = {}) const -> connection;

    template<partially_callable<Args...> Callable, execution_policy Policy = synchronous_policy>
    auto connect_once(Callable&& callable, Policy&& policy = {}) const -> connection;

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

    // TODO AROSS: Find a way to clean all the signals from emitting sources.
    void add_emitting_source(connection source) const
    {
        m_emitting_sources.emplace_back(std::move(source));
    }

    mutable std::vector<std::shared_ptr<connection_holder_implementation>> m_slots {};
    mutable std::vector<scoped_connection> m_emitting_sources {};
};

// ### map class
// TODO AROSS: Change connectable name

template<source_like Source, std::size_t... Indexes>
    requires((in_tuple_range<Indexes, typename std::remove_cvref_t<Source>::args> && ...) &&
             all_different<Indexes...>)
class mapped_source: public source,
                     public connectable2
{
public:
    friend connectable2;

    using args =
        std::tuple<std::tuple_element_t<Indexes, typename std::remove_cvref_t<Source>::args>&&...>;

    mapped_source(Source&& origin):
        m_source { std::forward<Source>(origin) }
    {
    }

private:
    template<partially_callable<
        std::tuple_element_t<Indexes, typename std::remove_cvref_t<Source>::args>...> Callable>
    auto forwarding_lambda(Callable&& callable) const
    {
        return [callable = std::forward<Callable>(callable)]<class... Args>(Args&&... args) mutable
        {
            partial_call(
                callable,
                std::forward<
                    std::tuple_element_t<Indexes, typename std::remove_cvref_t<Source>::args>>(
                    args...[Indexes])...);
        };
    }

    Source m_source;
};

template<std::size_t... Indexes>
    requires all_different<Indexes...>
class map
{
public:
    template<source_like Source>
    auto accept(Source&& origin) -> mapped_source<Source, Indexes...>
    {
        return { std::forward<Source>(origin) };
    }
};

// ### class transform

template<class Source, class... Transformations>
concept valid_transformations = requires(Transformations... transformations,
                                         typename std::remove_cvref_t<Source>::args args) {
    source_like<Source>;
    {
        [transformations, args]<std::size_t... Indexes>(const std::index_sequence<Indexes...>&)
        {
            (std::invoke(transformations...[Indexes], std::get<Indexes>(args)), ...);
        }(std::make_index_sequence<sizeof...(Transformations)> {})
    };
    [transformations,
     args]<std::size_t... Indexes>(const std::index_sequence<Indexes...>&) constexpr
    {
        return (not_void<std::invoke_result_t<
                    Transformations...[Indexes],
                    std::tuple_element_t<Indexes, typename std::remove_cvref_t<Source>::args>>> &&
                ...);
    }(std::make_index_sequence<sizeof...(Transformations)> {});
};

template<class Tuple, class... Transformations>
struct transformed_source_args;

template<class Tuple, class... Transformations>
using transformed_source_args_t = transformed_source_args<Tuple, Transformations...>::type;

template<class... Args, class... Transformations>
struct transformed_source_args<std::tuple<Args...>, Transformations...>
{
    using type = std::tuple<std::invoke_result_t<Transformations, Args>...>;
};

template<source_like Source, class... Transformations>
    requires(valid_transformations<Source, Transformations...>)
class transformed_source: public source,
                          public connectable2
{
public:
    friend connectable2;

    using args =
        transformed_source_args_t<typename std::remove_cvref_t<Source>::args, Transformations...>;

    transformed_source(Source&& origin, std::tuple<Transformations...> transformations):
        m_source { std::forward<Source>(origin) },
        m_transformations { std::move(transformations) }
    {
    }

private:
    template<partially_tuple_callable<args> Callable>
    auto forwarding_lambda(Callable&& callable) const
    {
        static constexpr std::make_index_sequence<sizeof...(Transformations)> index_sequence {};
        return [callable = std::forward<Callable>(callable),
                transformations = m_transformations]<class... Args>(Args&&... args) mutable
        {
            return [&callable, &transformations]<std::size_t... Indexes>(
                       const std::index_sequence<Indexes...>&,
                       Args&&... args) mutable
            {
                return partial_call(
                    callable,
                    std::get<Indexes>(transformations)(std::forward<Args>(args))...);
            }(index_sequence, std::forward<Args>(args)...);
        };
    }

    Source m_source;
    std::tuple<Transformations...> m_transformations;
};

template<class Source, class... Transformations>
transformed_source(Source&&, std::tuple<Transformations...>)
    -> transformed_source<Source, Transformations...>;

template<class... Transformations>
class transform
{
private:
    template<std::size_t>
    using identity = std::identity;

public:
    transform(Transformations... transformations):
        m_transformations { std::forward<Transformations>(transformations)... }
    {
    }

    template<source_like Source>
    auto accept(Source&& origin)
    {
        static constexpr std::make_index_sequence<
            std::tuple_size_v<typename std::remove_cvref_t<Source>::args> -
            sizeof...(Transformations)>
            index_sequence {};

        static constexpr auto identites { []<std::size_t... Indexes>(
                                              const std::index_sequence<Indexes...>&) constexpr
        { return std::make_tuple(identity<Indexes> {}...); }(index_sequence) };

        return transformed_source { std::forward<Source>(origin),
                                    std::tuple_cat(m_transformations, identites) };
    }

private:
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
    template<partially_callable<Args...> Callable, execution_policy Policy>
    connection_holder_implementation(const signal& connected_signal,
                                     Callable&& callable,
                                     Policy&& policy,
                                     bool single_shot = false):
        m_slot { [callable = std::forward<Callable>(callable)](Args&&... args) mutable
    { partial_call(callable, std::forward<Args>(args)...); } },
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

template<class Receiver, signal_arg... ReceiverArgs, source_like Emitter, execution_policy Policy>
    requires(partially_tuple_callable<std::function<void(ReceiverArgs...)>,
                                      typename std::remove_cvref_t<Emitter>::args>)
auto emitter::connect(this const Receiver& self,
                      Emitter&& emitter,
                      signal<ReceiverArgs...> Receiver::* receiver_signal,
                      Policy&& policy) -> connection
{
    auto connection { emitter.connect(self.forwarding_lambda(receiver_signal),
                                      std::forward<Policy>(policy)) };
    (self.*receiver_signal).add_emitting_source(connection);
    return connection;
}

template<class Receiver, signal_arg... ReceiverArgs, source_like Emitter, execution_policy Policy>
    requires(partially_tuple_callable<std::function<void(ReceiverArgs...)>,
                                      typename std::remove_cvref_t<Emitter>::args>)
auto emitter::connect_once(this const Receiver& self,
                           Emitter&& emitter,
                           signal<ReceiverArgs...> Receiver::* receiver_signal,
                           Policy&& policy) -> connection
{
    auto connection { emitter.connect_once(self.forwarding_lambda(receiver_signal),
                                           std::forward<Policy>(policy)) };
    (self.*receiver_signal).add_emitting_source(connection);
    return connection;
}

template<class Receiver, signal_arg... ReceiverArgs>
auto emitter::forwarding_lambda(this const Receiver& self,
                                signal<ReceiverArgs...> Receiver::* receiver_signal)
{
    return [&emitted_signal = self.*receiver_signal]<signal_arg... Args>(Args&&... args) mutable
        requires partially_callable<std::function<void(ReceiverArgs...)>, Args...>
    {
        auto lambda { [&]<class... CallArgs>(CallArgs&&... call_args) mutable
                          requires(sizeof...(CallArgs) == sizeof...(ReceiverArgs))
        { emitted_signal.emit(std::forward<CallArgs>(call_args)...); } };
        partial_call(lambda, std::forward<Args>(args)...);
    };
}

// ### signal implementation

template<signal_arg... Args>
template<partially_callable<Args...> Callable, execution_policy Policy>
auto emitter::signal<Args...>::connect(Callable&& callable, Policy&& policy) const -> connection
{
    m_slots.emplace_back(
        std::make_shared<connection_holder_implementation>(*this,
                                                           std::forward<Callable>(callable),
                                                           std::forward<Policy>(policy)));

    return { m_slots.back() };
}

template<signal_arg... Args>
template<partially_callable<Args...> Callable, execution_policy Policy>
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
