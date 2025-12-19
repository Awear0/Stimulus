#ifndef STIMULUS_H_
#define STIMULUS_H_

#include <atomic>
#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace details
{
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
            std::forward<Invocable>(invocable)();
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
        explicit execution_policy_holder_implementation(Policy&& policy):
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
            explicit policy_visitor_executor(Callable&& callable):
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

        private:
            // It might be bad, but this is done on purpose.
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
            Callable&& m_callable;
        };

        struct policy_visitor_synchronicity_checker
        {
            static auto constexpr operator()(const synchronous_policy&) -> bool
            {
                return synchronous_policy::is_synchronous;
            }

            static auto operator()(
                const std::unique_ptr<execution_policy_holder_implementation_interface>& policy)
                -> bool
            {
                return policy->is_synchronous();
            }
        };

        static constexpr policy_visitor_synchronicity_checker synchronicity_checker {};

    public:
        explicit execution_policy_holder(const synchronous_policy& policy):
            m_policy { policy }
        {
        }

        template<execution_policy Policy>
        explicit execution_policy_holder(Policy&& policy):
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
    constexpr bool all_different_implementation_t {
        all_different_implementation<Values...>::value
    };

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
    struct is_partially_callable_with_argument_count<Callable, 0, Args...>
        : std::is_invocable<Callable>
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

    template<class Callable, class... Args>
        requires partially_callable<Callable, Args...>
    constexpr auto partial_call(Callable&& callable, Args&&... args)
    {
        if constexpr (std::invocable<Callable, Args...>)
        {
            return std::invoke(std::forward<Callable>(callable), std::forward<Args>(args)...);
        }
        else
        {
            auto&& [... first_args, last] { std::forward_as_tuple(std::forward<Args>(args)...) };
            return partial_call(std::forward<Callable>(callable),
                                std::forward<decltype(first_args)>(first_args)...);
        }
    }

    template<class Callable, class Tuple>
    struct partially_tuple_callable_impl: std::false_type
    {
    };

    template<class Callable, class... Args>
        requires partially_callable<Callable, Args...>
    struct partially_tuple_callable_impl<Callable, std::tuple<Args...>>: std::true_type
    {
    };

    template<class Callable, class Tuple>
    concept partially_tuple_callable = partially_tuple_callable_impl<Callable, Tuple>::value;

    template<is_tuple Tuple, partially_tuple_callable<Tuple> Callable>
    constexpr auto partial_tuple_call(Callable&& callable, Tuple&& tuple)
    {
        auto&& [... args] { std::forward<Tuple>(tuple) };
        return partial_call(std::forward<Callable>(callable),
                            std::forward<decltype(args)>(args)...);
    }

    template<class BasicLockable>
    concept basic_lockable = requires(BasicLockable lockable) {
        { lockable.lock() };
        { lockable.unlock() };
    };

    class fake_mutex
    {
    public:
        constexpr void lock()
        {
        }

        constexpr void unlock()
        {
        }
    };

    template<template<class> class PointerLike>
    concept shared_pointer_like = requires(PointerLike<int> pointer) {
        { *pointer } -> std::same_as<int&>;
        { pointer = PointerLike<int> {} } -> std::same_as<PointerLike<int>&>;
        { pointer == nullptr } -> std::convertible_to<bool>;
        { pointer == PointerLike<int> {} } -> std::convertible_to<bool>;
        { static_cast<bool>(pointer) };
        { PointerLike<int> { pointer } } -> std::same_as<PointerLike<int>>;
        { PointerLike<int> { std::move(pointer) } } -> std::same_as<PointerLike<int>>;
        { PointerLike<int> {} } -> std::same_as<PointerLike<int>>;
        { pointer.~PointerLike<int>() };
        requires not_void<typename PointerLike<int>::weak_type>;
        { pointer.get() } -> std::same_as<int*>;
    };

    struct pointer_holder
    {
        void* pointer { nullptr };
        std::size_t count { 0 };
        std::size_t weak_count { 0 };
    };

    template<class T>
    class unsafe_weak_pointer;

    template<class T>
    class unsafe_shared_pointer
    {
    public:
        friend class unsafe_weak_pointer<T>;

        template<class>
        friend class unsafe_shared_pointer;

        using weak_type = unsafe_weak_pointer<T>;

        unsafe_shared_pointer() = default;

        explicit unsafe_shared_pointer(std::nullptr_t)
        {
        }

        explicit unsafe_shared_pointer(T* pointer):
            m_holder {
                (pointer != nullptr)
                    ? new pointer_holder { .pointer = static_cast<void*>(pointer), .count = 1 }
                    : nullptr
        }
        {
        }

        unsafe_shared_pointer(const unsafe_shared_pointer& other):
            m_holder { other.m_holder }
        {
            increase();
        }

        unsafe_shared_pointer(unsafe_shared_pointer&& other) noexcept
        {
            std::swap(m_holder, other.m_holder);
        }

        auto operator=(const unsafe_shared_pointer& other) -> unsafe_shared_pointer&
        {
            if (&other == this)
            {
                return *this;
            }

            release();

            m_holder = other.m_holder;

            increase();
            return *this;
        }

        auto operator=(unsafe_shared_pointer&& other) noexcept -> unsafe_shared_pointer&
        {
            if (&other == this)
            {
                return *this;
            }

            release();

            m_holder = other.m_holder;
            other.m_holder = nullptr;

            return *this;
        }

        ~unsafe_shared_pointer()
        {
            release();
        }

        auto operator==(T* other) const -> bool
        {
            if (other == nullptr)
            {
                return m_holder == nullptr;
            }

            if (m_holder == nullptr)
            {
                return false;
            }

            return other == m_holder->pointer;
        }

        auto operator==(const unsafe_shared_pointer& other) const -> bool
        {
            return other.m_holder == m_holder;
        }

        auto operator==(std::nullptr_t) const -> bool
        {
            return m_holder == nullptr || m_holder->pointer == nullptr;
        }

        auto operator*() const -> T&
        {
            return *static_cast<T*>(m_holder->pointer);
        }

        auto get() const -> T*
        {
            if (m_holder == nullptr)
            {
                return nullptr;
            }

            return static_cast<T*>(m_holder->pointer);
        }

        auto operator->() const -> T*
        {
            return get();
        }

        explicit operator bool() const
        {
            return m_holder != nullptr && m_holder->pointer != nullptr;
        }

        template<class Base>
            requires std::derived_from<T, Base>
        explicit operator unsafe_shared_pointer<Base>()
        {
            return unsafe_shared_pointer<Base> { m_holder };
        }

        template<class Base>
            requires std::derived_from<T, Base>
        explicit operator unsafe_weak_pointer<Base>()
        {
            return unsafe_weak_pointer<Base> { unsafe_shared_pointer<Base> { m_holder } };
        }

    private:
        void increase()
        {
            if (m_holder != nullptr)
            {
                ++m_holder->count;
            }
        }

        void release()
        {
            if (m_holder == nullptr)
            {
                return;
            }

            --m_holder->count;
            if (m_holder->count == 0)
            {
                delete get();
                m_holder->pointer = nullptr;

                if (m_holder->weak_count == 0)
                {
                    delete m_holder;
                    m_holder = nullptr;
                }
            }
        }

        explicit unsafe_shared_pointer(pointer_holder* holder):
            m_holder { holder }
        {
            increase();
        }

        pointer_holder* m_holder { nullptr };
    };

    template<class T>
    class unsafe_weak_pointer
    {
    public:
        unsafe_weak_pointer() = default;

        explicit unsafe_weak_pointer(unsafe_shared_pointer<T> shared_pointer):
            m_holder { shared_pointer.m_holder }
        {
            increase();
        }

        unsafe_weak_pointer(const unsafe_weak_pointer& other):
            m_holder { other.m_holder }
        {
            increase();
        }

        unsafe_weak_pointer(unsafe_weak_pointer&& other) noexcept
        {
            std::swap(m_holder, other.m_holder);
        }

        auto operator=(const unsafe_weak_pointer& other) -> unsafe_weak_pointer&
        {
            if (&other == this)
            {
                return *this;
            }

            release();

            m_holder = other.m_holder;

            increase();

            return *this;
        }

        auto operator=(unsafe_weak_pointer&& other) noexcept -> unsafe_weak_pointer&
        {
            std::swap(m_holder, other.m_holder);

            other.m_holder = nullptr;

            return *this;
        }

        ~unsafe_weak_pointer()
        {
            release();
        }

        auto lock() const -> unsafe_shared_pointer<T>
        {
            if (m_holder == nullptr)
            {
                return unsafe_shared_pointer<T> {};
            }

            if (m_holder->count == 0)
            {
                return unsafe_shared_pointer<T> {};
            }

            return unsafe_shared_pointer<T> { m_holder };
        }

    private:
        void increase()
        {
            if (m_holder != nullptr)
            {
                ++m_holder->weak_count;
            }
        }

        void release()
        {
            if (m_holder == nullptr)
            {
                return;
            }

            --m_holder->weak_count;
            if (m_holder->weak_count == 0 && m_holder->count == 0)
            {
                delete m_holder;
                m_holder = nullptr;
            }
        }

        pointer_holder* m_holder { nullptr };
    };

    template<details::basic_lockable Mutex = details::fake_mutex,
             template<class> class SharedPointer = details::unsafe_shared_pointer>
        requires details::shared_pointer_like<SharedPointer>
    class receiver;

} // namespace details

// ### Forward declaration

template<template<class> class SharedPointer>
    requires details::shared_pointer_like<SharedPointer>
class connection;
template<template<class> class SharedPointer>
    requires details::shared_pointer_like<SharedPointer>
class scoped_connection;

template<class Guard>
concept guard_like = requires(Guard guard_instance) {
    []<details::basic_lockable Mutex, template<class> class SharedPointer>(
        const details::receiver<Mutex, SharedPointer>&)
        requires details::shared_pointer_like<SharedPointer> {}(guard_instance);
};

template<class Callable,
         class Guard = void,
         details::execution_policy Policy = details::synchronous_policy>
    requires(std::same_as<void, Guard> || guard_like<Guard>)
class connect;
class chainable;

namespace details
{
    template<std::derived_from<chainable> Chainable,
             class Callable,
             class Guard,
             execution_policy Policy>
        requires(std::same_as<void, Guard> || guard_like<Guard>)
    class chain;
    template<template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    class source;
    template<details::basic_lockable Mutex = details::fake_mutex,
             template<class> class SharedPointer = details::unsafe_shared_pointer>
        requires details::shared_pointer_like<SharedPointer>
    class guard;

    template<class Instance>
    concept instance_of_source = requires(Instance instance) {
        []<template<class> class SharedPointer>(const source<SharedPointer>&)
            requires shared_pointer_like<SharedPointer>
        {

        }(instance);
    };
} // namespace details

template<class Source>
concept source_like =
    requires(const Source& source, typename std::remove_cvref_t<Source>::args tuple) {
        details::instance_of_source<std::remove_cvref_t<Source>>;
        details::is_tuple<typename std::remove_cvref_t<Source>::args>;
        {
            []<class... Args>(const std::tuple<Args...>&)
            { source.connect(std::function<void(Args...)> {}); }(tuple)
        };
        {
            []<class... Args>(const std::tuple<Args...>&)
            { source.connect_once(std::function<void(Args...)> {}); }(tuple)
        };
        requires details::not_void<typename std::remove_cvref_t<Source>::connection_type>;
        requires details::not_void<typename std::remove_cvref_t<Source>::connectable_type>;
    };

namespace details
{
    template<class Receiver, class MemberSignal, execution_policy Policy>
    class forward_result;

    template<class Receiver, class Signal, execution_policy Policy>
    class forward_result<Receiver, Signal Receiver::*, Policy>
    {
    public:
        forward_result(const Receiver& receiver,
                       Signal Receiver::* receiver_signal,
                       Policy&& policy,
                       bool connect_once):
            m_receiver { receiver },
            m_receiver_signal { receiver_signal },
            m_policy { std::forward<Policy>(policy) },
            m_connect_once { connect_once }
        {
        }

        template<source_like Source>
        auto create_connection(Source&& origin) ->
            typename std::remove_cvref_t<Source>::connection_type;

    private:
        // It might be bad, but this is done on purpose.
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
        const Receiver& m_receiver;
        Signal Receiver::* m_receiver_signal;
        Policy m_policy;
        bool m_connect_once;
    };

    template<class... Tuples>
    struct tuple_cat_result;

    template<class... Tuples>
    using tuple_cat_result_t = tuple_cat_result<Tuples...>::type;

    template<>
    struct tuple_cat_result<>
    {
        using type = std::tuple<>;
    };

    template<class... Args>
    struct tuple_cat_result<std::tuple<Args...>>
    {
        using type = std::tuple<Args...>;
    };

    template<class... LhsArgs, class... RhsArgs>
    struct tuple_cat_result<std::tuple<LhsArgs...>, std::tuple<RhsArgs...>>
    {
        using type = std::tuple<LhsArgs..., RhsArgs...>;
    };

    template<class Lhs, class Rhs, class... Others>
    struct tuple_cat_result<Lhs, Rhs, Others...>
    {
        using type = tuple_cat_result_t<tuple_cat_result_t<Lhs, Rhs>, Others...>;
    };

    template<class Self, class Callable, class Guard>
    concept callable_with_no_guard = requires {
        std::same_as<void, Guard>;
        partially_tuple_callable<Callable, typename std::remove_cvref_t<Self>::args>;
    };

    template<class Self, class Callable, class Guard>
    concept callable_with_guard = requires {
        guard_like<Guard>;
        partially_tuple_callable<Callable, typename std::remove_cvref_t<Self>::args>;
    };

    template<class Self, class Callable, class Guard>
    concept method_with_guard = requires {
        guard_like<Guard>;
        std::is_member_function_pointer_v<Callable>;
        partially_tuple_callable<
            Callable,
            tuple_cat_result_t<std::tuple<Guard>, typename std::remove_cvref_t<Self>::args>>;
    };

    // ### Class emitter declaration

    template<basic_lockable Mutex = fake_mutex,
             template<class> class SharedPointer = unsafe_shared_pointer>
        requires shared_pointer_like<SharedPointer>
    class emitter
    {
    public:
        friend class connection<SharedPointer>;
        friend class scoped_connection<SharedPointer>;
        friend class guard<Mutex, SharedPointer>;
        friend class source<SharedPointer>;
        friend class ::chainable;
        template<std::derived_from<chainable> Chainable,
                 class Callable,
                 class Guard,
                 execution_policy Policy>
            requires(std::same_as<void, Guard> || guard_like<Guard>)
        friend class chain;
        template<class Receiver, class MemberSignal, execution_policy Policy>
        friend class forward_result;

        emitter() = default;

        emitter(const emitter&) = default;
        emitter(emitter&&) = default;

        auto operator=(const emitter&) -> emitter& = default;
        auto operator=(emitter&&) -> emitter& = default;

        virtual ~emitter() = default;

    protected:
        template<signal_arg... Args>
        class signal;

        template<class Emitter, signal_arg... Args, class... EmittedArgs>
            requires std::invocable<typename signal<Args...>::slot, EmittedArgs&&...>
        void emit(this const Emitter& self,
                  signal<Args...> Emitter::* emitted_signal,
                  EmittedArgs&&... emitted_args)
        {
            (self.*emitted_signal).emit(std::forward<EmittedArgs>(emitted_args)...);
        }

        template<class Receiver,
                 signal_arg... ReceiverArgs,
                 source_like Emitter,
                 execution_policy Policy = synchronous_policy>
            requires(partially_tuple_callable<typename signal<ReceiverArgs...>::slot,
                                              typename std::remove_cvref_t<Emitter>::args>)
        auto connect(this const Receiver& self,
                     Emitter&& emitter,
                     signal<ReceiverArgs...> Receiver::* receiver_signal,
                     Policy&& policy = {}) ->
            typename std::remove_cvref_t<Emitter>::connection_type;

        template<class Receiver,
                 signal_arg... ReceiverArgs,
                 source_like Emitter,
                 execution_policy Policy = synchronous_policy>
            requires(partially_tuple_callable<typename signal<ReceiverArgs...>::slot,
                                              typename std::remove_cvref_t<Emitter>::args>)
        auto connect_once(this const Receiver& self,
                          Emitter&& emitter,
                          signal<ReceiverArgs...> Receiver::* receiver_signal,
                          Policy&& policy = {}) ->
            typename std::remove_cvref_t<Emitter>::connection_type;

        template<class Receiver, class Signal, execution_policy Policy = synchronous_policy>
        auto forward_to(this const Receiver& self,
                        Signal Receiver::* receiver_signal,
                        Policy&& policy = {})
            -> forward_result<Receiver, Signal Receiver::*, Policy>;

        template<class Receiver, class Signal, execution_policy Policy = synchronous_policy>
        auto forward_once_to(this const Receiver& self,
                             Signal Receiver::* receiver_signal,
                             Policy&& policy = {})
            -> forward_result<Receiver, Signal Receiver::*, Policy>;

    private:
        template<class Receiver, details::signal_arg... ReceiverArgs>
        auto forwarding_lambda(this const Receiver& self,
                               signal<ReceiverArgs...> Receiver::* receiver_signal);
    };

    template<class Appliable, class Source>
    concept appliable = requires(const Source& source, Appliable appliable) {
        ::source_like<Source>;
        { appliable.accept(source) } -> ::source_like;
    };

    template<class EmitterLike>
    concept emitter_like = requires(EmitterLike emitter_instance) {
        []<basic_lockable Mutex, template<class> class SharedPointer>(
            const emitter<Mutex, SharedPointer>&)
            requires shared_pointer_like<SharedPointer> {}(emitter_instance);
    };

    template<class SignalLike, class Emitter>
    concept signal_like = requires(SignalLike signal_instance) {
        requires emitter_like<Emitter>;
        []<signal_arg... Args>(const typename Emitter::template signal<Args...>&) {}(
            signal_instance);
    };

    class connection_holder;

    // ### Class source
    template<template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    class source
    {
    public:
        using connection_type = connection<SharedPointer>;

        template<::source_like Self, appliable<Self> Appliable>
        auto apply(this Self&& self, Appliable&& appliable)
        {
            return std::forward<Appliable>(appliable).accept(std::forward<Self>(self));
        }

        template<::source_like Self, appliable<Self> Appliable>
        auto operator|(this Self&& self, Appliable&& appliable)
        {
            return std::forward<Self>(self).apply(std::forward<Appliable>(appliable));
        }

        template<::source_like Self, class Callable, class Guard, execution_policy Policy>
            requires(callable_with_no_guard<Self, Callable, Guard> ||
                     callable_with_guard<Self, Callable, Guard> ||
                     method_with_guard<Self, Callable, Guard>)
        auto operator|(this Self&& self, connect<Callable, Guard, Policy>& connect)
        {
            return connect.create_connection(std::forward<Self>(self));
        }

        template<::source_like Self, class Callable, class Guard, execution_policy Policy>
            requires(callable_with_no_guard<Self, Callable, Guard> ||
                     callable_with_guard<Self, Callable, Guard> ||
                     method_with_guard<Self, Callable, Guard>)
        auto operator|(this Self&& self, connect<Callable, Guard, Policy>&& connect)
        {
            return std::move(connect).create_connection(std::forward<Self>(self));
        }

        template<::source_like Self,
                 class Receiver,
                 signal_like<Receiver> Signal,
                 execution_policy Policy>
            requires partially_tuple_callable<typename Signal::slot,
                                              typename std::remove_cvref_t<Self>::args>
        auto operator|(
            this Self&& self,
            details::template forward_result<Receiver, Signal Receiver::*, Policy>& connect_result)
        {
            return connect_result.create_connection(std::forward<Self>(self));
        }

        template<::source_like Self,
                 class Receiver,
                 signal_like<Receiver> Signal,
                 execution_policy Policy>
            requires partially_tuple_callable<typename Signal::slot,
                                              typename std::remove_cvref_t<Self>::args>
        auto operator|(
            this Self&& self,
            details::template forward_result<Receiver, Signal Receiver::*, Policy>&& connect_result)
        {
            return std::move(connect_result).create_connection(std::forward<Self>(self));
        }
    };

    class connection_holder
    {
    public:
        using exception_handler = std::function<void(std::exception_ptr)>;

        virtual ~connection_holder() = default;
        virtual void add_exception_handler(connection_holder::exception_handler handler) = 0;

        virtual void disconnect() = 0;
        virtual void suspend() = 0;
        virtual void resume() = 0;
    };

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<class Receiver, class Signal, execution_policy Policy>
    auto emitter<Mutex, SharedPointer>::forward_to(this const Receiver& self,
                                                   Signal Receiver::* receiver_signal,
                                                   Policy&& policy)
        -> forward_result<Receiver, Signal Receiver::*, Policy>
    {
        return { self, receiver_signal, std::forward<Policy>(policy), false };
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<class Receiver, class Signal, execution_policy Policy>
    auto emitter<Mutex, SharedPointer>::forward_once_to(this const Receiver& self,
                                                        Signal Receiver::* receiver_signal,
                                                        Policy&& policy)
        -> forward_result<Receiver, Signal Receiver::*, Policy>
    {
        return { self, receiver_signal, std::forward<Policy>(policy), true };
    }

} // namespace details

// ### Connection related classes

template<template<class> class SharedPointer>
    requires details::shared_pointer_like<SharedPointer>
class connection
{
public:
    explicit connection(typename SharedPointer<details::connection_holder>::weak_type weak_holder):
        m_holder { std::move(weak_holder) }
    {
    }

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

    void add_exception_handler(details::connection_holder::exception_handler handler)
    {
        auto locked_holder { m_holder.lock() };
        if (!locked_holder)
        {
            return;
        }

        locked_holder->add_exception_handler(std::move(handler));
    }

    auto operator==(details::connection_holder* holder) -> bool
    {
        return m_holder.lock().get() == holder;
    }

private:
    typename SharedPointer<details::connection_holder>::weak_type m_holder;
};

template<template<class> class SharedPointer>
    requires details::shared_pointer_like<SharedPointer>
class scoped_connection
{
public:
    // Not explicit on purpose to allow scoped_connection = object.signal.connect(slot);
    // NOLINTNEXTLINE(google-explicit-constructor)
    scoped_connection(connection<SharedPointer> conn):
        m_connection { std::move(conn) }
    {
    }

    scoped_connection(const scoped_connection&) = delete;

    scoped_connection(scoped_connection&& other) noexcept:
        m_connection { std::move(other.m_connection) }
    {
    }

    auto operator=(const scoped_connection&) -> scoped_connection& = delete;

    auto operator=(scoped_connection&& other) noexcept -> scoped_connection&
    {
        m_connection = std::move(other.m_connection);
        return *this;
    }

    ~scoped_connection()
    {
        disconnect();
    }

    void disconnect()
    {
        m_connection.disconnect();
    }

    auto operator==(details::connection_holder* holder) -> bool
    {
        return m_connection == holder;
    }

private:
    connection<SharedPointer> m_connection;
};

template<template<class> class SharedPointer>
    requires details::shared_pointer_like<SharedPointer>
class inhibitor
{
public:
    explicit inhibitor(connection<SharedPointer> conn):
        m_connection { std::move(conn) }
    {
        m_connection.suspend();
    }

    inhibitor(const inhibitor&) = default;
    inhibitor(inhibitor&&) = default;

    auto operator=(const inhibitor&) -> inhibitor& = default;
    auto operator=(inhibitor&&) -> inhibitor& = default;

    ~inhibitor()
    {
        m_connection.resume();
    }

private:
    connection<SharedPointer> m_connection;
};

// ### Class receiver

namespace details
{
    template<details::basic_lockable Mutex, template<class> class SharedPointer>
        requires details::shared_pointer_like<SharedPointer>
    class guard
    {
    public:
        template<details::signal_arg... Args>
        friend class emitter<Mutex, SharedPointer>::signal;

        guard() = default;
        ~guard() = default;

        guard(const guard&)
        {
            // Empty on purpose
        }

        guard(guard&&) noexcept
        {
            // Nothing on purpose
        }

        auto operator=(const guard&) -> guard&
        {
            // Nothing on purpose
            return *this;
        }

        auto operator=(guard&&) noexcept -> guard&
        {
            // Nothing on purpose
            return *this;
        }

    protected:
        void clean(details::connection_holder* holder) const
        {
            std::lock_guard lock { m_mutex };
            std::erase(m_emitting_sources, holder);
        }

        void add_emitting_source(connection<SharedPointer> source) const
        {
            std::lock_guard lock { m_mutex };
            m_emitting_sources.emplace_back(std::move(source));
        }

    private:
        mutable std::vector<scoped_connection<SharedPointer>> m_emitting_sources;
        mutable Mutex m_mutex;
    };

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    class receiver: public guard<Mutex, SharedPointer>
    {
    public:
        template<signal_arg... Args>
        friend class emitter<Mutex, SharedPointer>::signal;
    };

} // namespace details

// ### connectable

template<template<class> class SharedPointer>
    requires details::shared_pointer_like<SharedPointer>
class connectable: public details::source<SharedPointer>
{
public:
    using connectable_type = connectable<SharedPointer>;

    template<class Self,
             class Callable,
             details::execution_policy Policy = details::synchronous_policy>
    auto connect(this Self&& self, Callable&& callable, Policy&& policy = {})
        -> connection<SharedPointer>
    {
        return std::forward<Self>(self).m_source.connect(
            std::forward<Self>(self).forwarding_lambda(std::forward<Callable>(callable)),
            std::forward<Policy>(policy));
    }

    template<class Self,
             class Callable,
             details::execution_policy Policy = details::synchronous_policy>
    auto connect_once(this Self&& self, Callable&& callable, Policy&& policy = {})
        -> connection<SharedPointer>
    {
        return std::forward<Self>(self).m_source.connect_once(
            std::forward<Self>(self).forwarding_lambda(std::forward<Callable>(callable)),
            std::forward<Policy>(policy));
    }

    template<class Self,
             class Callable,
             guard_like Receiver,
             details::execution_policy Policy = details::synchronous_policy>
    auto connect(this Self&& self, Callable&& callable, const Receiver& guard, Policy&& policy = {})
        -> connection<SharedPointer>
    {
        return std::forward<Self>(self).m_source.connect(
            std::forward<Self>(self).forwarding_lambda(std::forward<Callable>(callable)),
            guard,
            std::forward<Policy>(policy));
    }

    template<class Self,
             class Callable,
             guard_like Receiver,
             details::execution_policy Policy = details::synchronous_policy>
    auto connect_once(this Self&& self,
                      Callable&& callable,
                      const Receiver& guard,
                      Policy&& policy = {}) -> connection<SharedPointer>
    {
        return std::forward<Self>(self).m_source.connect_once(
            std::forward<Self>(self).forwarding_lambda(std::forward<Callable>(callable)),
            guard,
            std::forward<Policy>(policy));
    }

    template<class Self,
             guard_like Receiver,
             class Result,
             details::execution_policy Policy = details::synchronous_policy,
             class... MemberFunctionArgs>
    auto connect(this Self&& self,
                 Result (Receiver::*callable)(MemberFunctionArgs...),
                 Receiver& guard,
                 Policy&& policy = {}) -> connection<SharedPointer>
    {
        return std::forward<Self>(self).m_source.connect(
            std::forward<Self>(self).forwarding_lambda(
                [&guard, callable]<class... Args>(Args&&... args) mutable
        { partial_call(callable, guard, std::forward<Args>(args)...); }),
            guard,
            std::forward<Policy>(policy));
    }

    template<class Self,
             guard_like Receiver,
             class Result,
             details::execution_policy Policy = details::synchronous_policy,
             class... MemberFunctionArgs>
    auto connect_once(this Self&& self,
                      Result (Receiver::*callable)(MemberFunctionArgs...),
                      Receiver& guard,
                      Policy&& policy = {}) -> connection<SharedPointer>
    {
        return std::forward<Self>(self).m_source.connect_once(
            std::forward<Self>(self).forwarding_lambda(
                [&guard, callable]<class... Args>(Args&&... args) mutable
        { partial_call(callable, guard, std::forward<Args>(args)...); }),
            guard,
            std::forward<Policy>(policy));
    }

    template<class Self,
             guard_like Receiver,
             class Result,
             details::execution_policy Policy = details::synchronous_policy,
             class... MemberFunctionArgs>
    auto connect(this Self&& self,
                 Result (Receiver::*callable)(MemberFunctionArgs...) const,
                 const Receiver& guard,
                 Policy&& policy = {}) -> connection<SharedPointer>
    {
        return std::forward<Self>(self).m_source.connect(
            std::forward<Self>(self).forwarding_lambda(
                [&guard, callable]<class... Args>(Args&&... args) mutable
        { partial_call(callable, guard, std::forward<Args>(args)...); }),
            guard,
            std::forward<Policy>(policy));
    }

    template<class Self,
             guard_like Receiver,
             class Result,
             details::execution_policy Policy = details::synchronous_policy,
             class... MemberFunctionArgs>
    auto connect_once(this Self&& self,
                      Result (Receiver::*callable)(MemberFunctionArgs...) const,
                      const Receiver& guard,
                      Policy&& policy = {}) -> connection<SharedPointer>
    {
        return std::forward<Self>(self).m_source.connect_once(
            std::forward<Self>(self).forwarding_lambda(
                [&guard, callable]<class... Args>(Args&&... args) mutable
        { partial_call(callable, guard, std::forward<Args>(args)...); }),
            guard,
            std::forward<Policy>(policy));
    }
};

// ### class chainable

class chainable;

namespace details
{
    // ### Signal definition

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    class emitter<Mutex, SharedPointer>::signal final: public source<SharedPointer>,
                                                       public guard<Mutex, SharedPointer>
    {
    public:
        using connection_type = connection<SharedPointer>;
        using connectable_type = connectable<SharedPointer>;

        signal() = default;

        signal(const signal&):
            signal()
        {
            // Nothing on purpose
        }

        signal(signal&&) noexcept
        {
            // Nothing on purpose
        }

        auto operator=(const signal&) -> signal&
        {
            // Nothing on purpose
            return *this;
        }

        auto operator=(signal&&) noexcept -> signal&
        {
            // Nothing on purpose
            return *this;
        }

        ~signal() = default;

        using args = std::tuple<Args...>;

        friend emitter;
        using slot = std::function<void(Args...)>;

        template<partially_callable<Args...> Callable, execution_policy Policy = synchronous_policy>
        auto connect(Callable&& callable, Policy&& policy = {}) const -> connection<SharedPointer>;

        template<partially_callable<Args...> Callable, execution_policy Policy = synchronous_policy>
        auto connect_once(Callable&& callable, Policy&& policy = {}) const
            -> connection<SharedPointer>;

        template<partially_callable<Args...> Callable,
                 guard_like Receiver,
                 execution_policy Policy = synchronous_policy>
        auto connect(Callable&& callable, const Receiver& guard, Policy&& policy = {}) const
            -> connection<SharedPointer>;

        template<partially_callable<Args...> Callable,
                 guard_like Receiver,
                 execution_policy Policy = synchronous_policy>
        auto connect_once(Callable&& callable, const Receiver& guard, Policy&& policy = {}) const
            -> connection<SharedPointer>;

        template<guard_like Receiver,
                 class Result,
                 execution_policy Policy = synchronous_policy,
                 class... MemberFunctionArgs>
            requires partially_callable<Result (Receiver::*)(MemberFunctionArgs...),
                                        Receiver,
                                        Args...>
        auto connect(Result (Receiver::*callable)(MemberFunctionArgs...),
                     Receiver& guard,
                     Policy&& policy = {}) const -> connection<SharedPointer>;

        template<guard_like Receiver,
                 class Result,
                 execution_policy Policy = synchronous_policy,
                 class... MemberFunctionArgs>
            requires partially_callable<Result (Receiver::*)(MemberFunctionArgs...),
                                        Receiver,
                                        Args...>
        auto connect_once(Result (Receiver::*callable)(MemberFunctionArgs...),
                          Receiver& guard,
                          Policy&& policy = {}) const -> connection<SharedPointer>;

        template<guard_like Receiver,
                 class Result,
                 execution_policy Policy = synchronous_policy,
                 class... MemberFunctionArgs>
            requires partially_callable<Result (Receiver::*)(MemberFunctionArgs...) const,
                                        const Receiver,
                                        Args...>
        auto connect(Result (Receiver::*callable)(MemberFunctionArgs...) const,
                     const Receiver& guard,
                     Policy&& policy = {}) const -> connection<SharedPointer>;

        template<guard_like Receiver,
                 class Result,
                 execution_policy Policy = synchronous_policy,
                 class... MemberFunctionArgs>
            requires partially_callable<Result (Receiver::*)(MemberFunctionArgs...) const,
                                        const Receiver,
                                        Args...>
        auto connect_once(Result (Receiver::*callable)(MemberFunctionArgs...) const,
                          const Receiver& guard,
                          Policy&& policy = {}) const -> connection<SharedPointer>;

    private:
        template<partially_callable<Args...> Callable, execution_policy Policy>
        auto connect_impl(Callable&& callable, Policy&& policy, bool connect_once) const
            -> connection<SharedPointer>;

        template<partially_callable<Args...> Callable, guard_like Receiver, execution_policy Policy>
        auto connect_with_guard(Callable&& callable,
                                const Receiver& guard,
                                Policy&& policy,
                                bool connect_once) const -> connection<SharedPointer>;

        template<guard_like Receiver, class MemberFunction>
            requires partially_callable<MemberFunction, Receiver, Args...>
        static auto member_function_lambda(MemberFunction member_function, Receiver& guard)
        {
            return [&guard, member_function]<class... CallArgs>(CallArgs&&... args) mutable
            { partial_call(member_function, guard, std::forward<CallArgs>(args)...); };
        }

        template<class... EmittedArgs>
            requires std::invocable<slot, EmittedArgs&&...>
        void emit(EmittedArgs&&... emitted_args) const
        {
            auto slots { copy_slots() };

            if (slots->empty())
            {
                return;
            }

            auto begin { slots->begin() };
            auto previous_to_end { std::prev(slots->end()) };

            for (auto it { begin }; it != previous_to_end; ++it)
            {
                (**it)(emitted_args...);
            }

            (*slots->back())(std::forward<EmittedArgs>(emitted_args)...);
        }

        class connection_holder_implementation;

        using slot_list = std::vector<SharedPointer<connection_holder_implementation>>;

        auto copy_slots() const -> SharedPointer<slot_list>
        {
            std::lock_guard lock { m_mutex };
            return m_slots;
        }

        void disconnect(connection_holder_implementation* holder) const
        {
            std::lock_guard lock { m_mutex };
            SharedPointer<slot_list> slots { new slot_list(*m_slots) };
            std::erase_if(*slots, [holder](const auto& slot) { return slot.get() == holder; });

            std::swap(slots, m_slots);
        }

        mutable SharedPointer<slot_list> m_slots { SharedPointer<slot_list>(new slot_list()) };
        mutable Mutex m_mutex;
    };

    template<std::derived_from<chainable> Chainable,
             class Callable,
             class Guard,
             execution_policy Policy>
        requires(std::same_as<void, Guard> || guard_like<Guard>)
    class chain
    {
    public:
        chain(Chainable&& chainable, const connect<Callable, Guard, Policy>& connect):
            m_chainable { std::forward<Chainable>(chainable) },
            m_connect { std::move(connect) }
        {
        }

        template<source_like Source>
        friend auto operator|(Source&& source, chain& chain)
        {
            return std::forward<Source>(source) | chain.m_chainable | chain.m_connect;
        }

    private:
        std::remove_cvref_t<Chainable> m_chainable;
        connect<Callable, Guard, Policy> m_connect;
    };

    template<std::derived_from<chainable> Chainable,
             class Receiver,
             signal_like<Receiver> Signal,
             details::execution_policy Policy>
    class chain<Chainable, Signal Receiver::*, void, Policy>
    {
    public:
        chain(Chainable&& chainable,
              const forward_result<Receiver, Signal Receiver::*, Policy>& connect_result):
            m_chainable { std::forward<Chainable>(chainable) },
            m_connect_result { std::move(connect_result) }
        {
        }

        template<source_like Source>
        friend auto operator|(Source&& source, chain& chain)
        {
            return std::forward<Source>(source) | chain.m_chainable | chain.m_connect_result;
        }

    private:
        std::remove_cvref_t<Chainable> m_chainable;
        forward_result<Receiver, Signal Receiver::*, Policy> m_connect_result;
    };

} // namespace details

class chainable
{
public:
    template<class Self, std::derived_from<chainable> OtherChainable>
    auto operator|(this Self&& self, OtherChainable&& other);

    template<class Self, class Callable, class Guard, details::execution_policy Policy>
        requires(std::same_as<void, Guard> || guard_like<Guard>)
    auto operator|(this Self&& self, const connect<Callable, Guard, Policy>& connect)
        -> details::chain<Self, Callable, Guard, Policy>
    {
        return { std::forward<Self>(self), connect };
    }

    template<class Self,
             class Receiver,
             details::signal_like<Receiver> Signal,
             details::execution_policy Policy>
    auto operator|(
        this Self&& self,
        const details::forward_result<Receiver, Signal Receiver::*, Policy>& connect_result)
        -> details::chain<Self, Signal Receiver::*, void, Policy>
    {
        return { std::forward<Self>(self), connect_result };
    }
};

namespace details
{
    template<std::derived_from<chainable> Lhs, std::derived_from<chainable> Rhs>
    class composed_chainable: public chainable
    {
    public:
        composed_chainable(Lhs&& lhs, Rhs&& rhs):
            m_lhs { std::forward<Lhs>(lhs) },
            m_rhs { std::forward<Rhs>(rhs) }
        {
        }

        template<source_like Source>
        friend auto operator|(Source&& source, composed_chainable& composed_chainable)
        {
            return (std::forward<Source>(source) | composed_chainable.m_lhs) |
                   composed_chainable.m_rhs;
        }

    private:
        Lhs m_lhs;
        Rhs m_rhs;
    };
} // namespace details

template<class Self, std::derived_from<chainable> OtherChainable>
auto chainable::operator|(this Self&& self, OtherChainable&& other)
{
    return details::composed_chainable { std::forward<Self>(self),
                                         std::forward<OtherChainable>(other) };
}

// ### class map

namespace details
{
    template<class Source, std::size_t... Indexes>
    concept valid_map = requires {
        ::source_like<Source>;
        (in_tuple_range<Indexes, typename std::remove_cvref_t<Source>::args> && ...);
        all_different<Indexes...>;
    };

    template<::source_like Source, std::size_t... Indexes>
        requires(valid_map<Source, Indexes...>)
    class mapped_source: public std::remove_cvref_t<Source>::connectable_type
    {
    public:
        friend std::remove_cvref_t<Source>::connectable_type;

        using args = std::tuple<
            std::tuple_element_t<Indexes, typename std::remove_cvref_t<Source>::args>...>;

        explicit mapped_source(Source&& origin):
            m_source { std::forward<Source>(origin) }
        {
        }

    private:
        template<partially_callable<
            std::tuple_element_t<Indexes, typename std::remove_cvref_t<Source>::args>...> Callable>
        auto forwarding_lambda(Callable&& callable) const
        {
            return
                // Clang-tidy doesn't seem to understand template parameter pack indexing.
                // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
                [callable = std::forward<Callable>(callable)]<class... Args>(Args&&... args) mutable
            {
                partial_call(callable,
                             std::forward<decltype(args...[Indexes])>(args...[Indexes])...);
            };
        }

        Source m_source;
    };
} // namespace details

template<std::size_t... Indexes>
    requires details::all_different<Indexes...>
class map: public chainable
{
public:
    template<source_like Source>
    auto accept(Source&& origin) -> details::mapped_source<Source, Indexes...>
    {
        return details::mapped_source<Source, Indexes...> { std::forward<Source>(origin) };
    }
};

// ### class transform

namespace details
{
    template<class Source, class... Transformations>
    concept valid_transformations = requires(Transformations... transformations,
                                             typename std::remove_cvref_t<Source>::args args) {
        ::source_like<Source>;
        [transformations,
         args]<std::size_t... Indexes>(const std::index_sequence<Indexes...>&) constexpr
            requires(
                (std::invocable<
                     Transformations...[Indexes],
                     std::tuple_element_t<Indexes, typename std::remove_cvref_t<Source>::args>> &&
                 ...) &&
                (not_void<std::invoke_result_t<
                     Transformations...[Indexes],
                     std::tuple_element_t<Indexes, typename std::remove_cvref_t<Source>::args>>> &&
                 ...)) {}(std::make_index_sequence<sizeof...(Transformations)> {});
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

    template<::source_like Source, class... Transformations>
        requires(valid_transformations<Source, Transformations...>)
    class transformed_source: public std::remove_cvref_t<Source>::connectable_type
    {
    public:
        friend std::remove_cvref_t<Source>::connectable_type;

        using args = transformed_source_args_t<typename std::remove_cvref_t<Source>::args,
                                               Transformations...>;

        transformed_source(Source&& origin, std::tuple<Transformations...> transformations):
            m_source { std::forward<Source>(origin) },
            m_transformations { std::move(transformations) }
        {
        }

    private:
        template<partially_tuple_callable<args> Callable>
        auto forwarding_lambda(Callable&& callable) const
        {
            return [callable = std::forward<Callable>(callable),
                    transformations = m_transformations]<class... Args>(Args&&... args) mutable
            {
                auto& [... transformationsCallable] { transformations };
                return partial_call(callable, transformationsCallable(std::forward<Args>(args))...);
            };
        }

        Source m_source;
        std::tuple<Transformations...> m_transformations;
    };

    template<class Source, class... Transformations>
    transformed_source(Source&&, std::tuple<Transformations...>)
        -> transformed_source<Source, Transformations...>;
} // namespace details

template<class... Transformations>
class transform: public chainable
{
private:
    template<std::size_t>
    using identity = std::identity;

public:
    explicit transform(Transformations... transformations):
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

        auto [... identities] { create_identities(index_sequence) };
        auto& [... transformations] { m_transformations };

        return details::transformed_source { std::forward<Source>(origin),
                                             std::make_tuple(transformations..., identities...) };
    }

private:
    template<std::size_t... Indexes>
    static constexpr auto create_identities(const std::index_sequence<Indexes...>&)
    {
        return std::make_tuple(identity<Indexes> {}...);
    }

    std::tuple<Transformations...> m_transformations;
};

// ### class filter

namespace details
{
    template<class Source, class Filter>
    concept valid_filter = requires {
        ::source_like<Source>;
        partially_tuple_callable<Filter, typename std::remove_cvref_t<Source>::args>;
        std::convertible_to<decltype(partial_tuple_call(
                                std::declval<Filter>(),
                                std::declval<typename std::remove_cvref_t<Source>::args>())),
                            bool>;
    };

    template<::source_like Source,
             partially_tuple_callable<typename std::remove_cvref_t<Source>::args> Filter>
        requires valid_filter<Source, Filter>
    class filtered_source: public std::remove_cvref_t<Source>::connectable_type
    {
    public:
        friend std::remove_cvref_t<Source>::connectable_type;

        using args = typename std::remove_cvref_t<Source>::args;

        filtered_source(Source&& origin, Filter filter):
            m_source { std::forward<Source>(origin) },
            m_filter { std::move(filter) }
        {
        }

    private:
        template<partially_tuple_callable<args> Callable>
        auto forwarding_lambda(Callable&& callable) const
        {
            return [callable = std::forward<Callable>(callable),
                    filter = m_filter]<class... Args>(Args&&... args) mutable
            {
                if (static_cast<bool>(partial_call(filter, args...)))
                {
                    partial_call(callable, std::forward<Args>(args)...);
                }
            };
        }

        Source m_source;
        Filter m_filter;
    };
} // namespace details

template<class Filter>
class filter: public chainable
{
public:
    explicit filter(Filter filter):
        m_filter { std::move(filter) }
    {
    }

    template<source_like Source>
    auto accept(Source&& origin) -> details::filtered_source<Source, Filter>
    {
        return { std::forward<Source>(origin), m_filter };
    }

private:
    Filter m_filter;
};

// ### connection_holder_implementation class

namespace details
{

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    class emitter<Mutex, SharedPointer>::signal<Args...>::connection_holder_implementation final
        : public connection_holder
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
            m_slot { generate_slot(std::forward<Callable>(callable)) },
            m_signal { connected_signal },
            m_policy(std::forward<Policy>(policy)),
            m_single_shot { single_shot }
        {
        }

        template<partially_callable<Args...> Callable, execution_policy Policy>
        connection_holder_implementation(const signal& connected_signal,
                                         Callable&& callable,
                                         Policy&& policy,
                                         guard<Mutex, SharedPointer>& guard,
                                         bool single_shot = false):
            connection_holder_implementation(connected_signal,
                                             std::forward<Callable>(callable),
                                             std::forward<Policy>(policy),
                                             single_shot),
            m_guard { &guard }
        {
        }

        template<class... EmittedArgs>
            requires std::invocable<signal::slot, EmittedArgs...>
        void operator()(EmittedArgs&&... args)
        {
            if (m_suspended.load(std::memory_order_relaxed))
            {
                return;
            }
            if (m_single_shot)
            {
                disconnect();
            }

            auto exception_handlers { copy_exception_handlers() };
            if (m_policy.is_synchronous())
            {
                m_policy.execute([&]
                { safe_execute(m_slot, exception_handlers, std::forward<EmittedArgs>(args)...); });
            }
            else
            {
                m_policy.execute(
                    [exception_handlers = copy_exception_handlers(),
                     slot = m_slot,
                     ... args = ref_or_value<Args>(std::forward<EmittedArgs>(args))] mutable
                { safe_execute(slot, exception_handlers, std::forward<decltype(args)>(args)...); });
            }
        }

        template<class... ExecuteArgs>
        static void safe_execute(signal<Args...>::slot& slot,
                                 std::vector<exception_handler>& exception_handlers,
                                 ExecuteArgs&&... execute_args)
        {
            try
            {
                slot(std::forward<ExecuteArgs>(execute_args)...);
            }
            catch (...)
            {
                if (exception_handlers.empty())
                {
                    throw;
                }
                auto current_exception { std::current_exception() };
                for (auto& handler: exception_handlers)
                {
                    // Exception handlers shouldn't throw. If they do, that's not on us.
                    handler(current_exception);
                }
            }
        }

        void disconnect() override
        {
            m_signal.disconnect(this);
            if (m_guard != nullptr)
            {
                guard<Mutex, SharedPointer>* guard { nullptr };
                std::swap(guard, m_guard);
                guard->clean(this);
            }
        }

        void suspend() override
        {
            m_suspended.store(true, std::memory_order_relaxed);
        }

        void resume() override
        {
            m_suspended.store(false, std::memory_order_relaxed);
        }

        void add_exception_handler(connection_holder::exception_handler handler) override
        {
            std::lock_guard lock { m_mutex };
            m_exception_handlers.emplace_back(std::move(handler));
        }

    private:
        auto copy_exception_handlers() const -> std::vector<exception_handler>
        {
            std::lock_guard lock { m_mutex };

            return m_exception_handlers;
        }

        template<partially_callable<Args...> Callable>
        static auto generate_slot(Callable&& callable)
        {
            return [callable = std::forward<Callable>(callable)](Args&&... args) mutable
            { partial_call(callable, std::forward<Args>(args)...); };
        }

        signal::slot m_slot;
        std::vector<exception_handler> m_exception_handlers;
        mutable Mutex m_mutex;
        // It might be bad, but this is done on purpose.
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
        const signal& m_signal;
        guard<Mutex, SharedPointer>* m_guard { nullptr };
        execution_policy_holder m_policy;
        std::atomic<bool> m_suspended { false };
        bool m_single_shot;
    };

    // ### emitter implementation

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<class Receiver,
             signal_arg... ReceiverArgs,
             source_like Emitter,
             execution_policy Policy>
        requires(partially_tuple_callable<
                 typename emitter<Mutex, SharedPointer>::template signal<ReceiverArgs...>::slot,
                 typename std::remove_cvref_t<Emitter>::args>)
    auto emitter<Mutex, SharedPointer>::connect(
        this const Receiver& self,
        Emitter&& emitter,
        ::details::emitter<Mutex, SharedPointer>::template signal<ReceiverArgs...> Receiver::*
            receiver_signal,
        Policy&& policy) -> typename std::remove_cvref_t<Emitter>::connection_type
    {
        auto connection { std::forward<Emitter>(emitter).connect(
            self.forwarding_lambda(receiver_signal),
            std::forward<Policy>(policy)) };
        (self.*receiver_signal).add_emitting_source(connection);
        return connection;
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<class Receiver,
             signal_arg... ReceiverArgs,
             source_like Emitter,
             execution_policy Policy>
        requires(partially_tuple_callable<
                 typename emitter<Mutex, SharedPointer>::template signal<ReceiverArgs...>::slot,
                 typename std::remove_cvref_t<Emitter>::args>)
    auto emitter<Mutex, SharedPointer>::connect_once(
        this const Receiver& self,
        Emitter&& emitter,
        signal<ReceiverArgs...> Receiver::* receiver_signal,
        Policy&& policy) -> typename std::remove_cvref_t<Emitter>::connection_type
    {
        auto connection { std::forward<Emitter>(emitter).connect_once(
            self.forwarding_lambda(receiver_signal),
            std::forward<Policy>(policy)) };
        (self.*receiver_signal).add_emitting_source(connection);
        return connection;
    }

    template<class Receiver, class Signal, execution_policy Policy>
    template<source_like Source>
    auto forward_result<Receiver, Signal Receiver::*, Policy>::create_connection(Source&& origin) ->
        typename std::remove_cvref_t<Source>::connection_type
    {
        if (m_connect_once)
        {
            return m_receiver.connect_once(std::forward<Source>(origin),
                                           m_receiver_signal,
                                           m_policy);
        }
        return m_receiver.connect(std::forward<Source>(origin), m_receiver_signal, m_policy);
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<class Receiver, signal_arg... ReceiverArgs>
    auto emitter<Mutex, SharedPointer>::forwarding_lambda(
        this const Receiver& self,
        signal<ReceiverArgs...> Receiver::* receiver_signal)
    {
        return [&emitted_signal = self.*receiver_signal]<signal_arg... Args>(Args&&... args) mutable
            requires partially_callable<std::function<void(ReceiverArgs...)>, Args...>
        {
            auto lambda { [&]<class... CallArgs>(CallArgs&&... call_args) mutable
                              requires(sizeof...(CallArgs) == sizeof...(ReceiverArgs))
            { emitted_signal.emit(std::forward<CallArgs>(call_args)...); } };
            details::partial_call(lambda, std::forward<Args>(args)...);
        };
    }

    // ### signal implementation

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<partially_callable<Args...> Callable, execution_policy Policy>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect_impl(Callable&& callable,
                                                                      Policy&& policy,
                                                                      bool connect_once) const
        -> connection<SharedPointer>
    {
        std::lock_guard lock { m_mutex };
        SharedPointer<slot_list> slots { new slot_list(*m_slots) };
        slots->emplace_back(new connection_holder_implementation(*this,
                                                                 std::forward<Callable>(callable),
                                                                 std::forward<Policy>(policy),
                                                                 connect_once));

        std::swap(slots, m_slots);

        return connection<SharedPointer> {
            typename SharedPointer<details::connection_holder>::weak_type(m_slots->back())
        };
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<partially_callable<Args...> Callable, execution_policy Policy>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect(Callable&& callable,
                                                                 Policy&& policy) const
        -> connection<SharedPointer>
    {
        return connect_impl(std::forward<Callable>(callable), std::forward<Policy>(policy), false);
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<partially_callable<Args...> Callable, execution_policy Policy>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect_once(Callable&& callable,
                                                                      Policy&& policy) const
        -> connection<SharedPointer>
    {
        return connect_impl(std::forward<Callable>(callable), std::forward<Policy>(policy), true);
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<partially_callable<Args...> Callable, guard_like Receiver, execution_policy Policy>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect_with_guard(Callable&& callable,
                                                                            const Receiver& guard,
                                                                            Policy&& policy,
                                                                            bool connect_once) const
        -> connection<SharedPointer>
    {
        auto connection { connect_impl(std::forward<Callable>(callable),
                                       std::forward<Policy>(policy),
                                       connect_once) };
        guard.add_emitting_source(connection);

        return connection;
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<partially_callable<Args...> Callable, guard_like Receiver, execution_policy Policy>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect(Callable&& callable,
                                                                 const Receiver& guard,
                                                                 Policy&& policy) const
        -> connection<SharedPointer>
    {
        return connect_with_guard(std::forward<Callable>(callable),
                                  guard,
                                  std::forward<Policy>(policy),
                                  false);
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<partially_callable<Args...> Callable, guard_like Receiver, execution_policy Policy>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect_once(Callable&& callable,
                                                                      const Receiver& guard,
                                                                      Policy&& policy) const
        -> connection<SharedPointer>
    {
        return connect_with_guard(std::forward<Callable>(callable),
                                  guard,
                                  std::forward<Policy>(policy),
                                  true);
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<guard_like Receiver,
             class Result,
             execution_policy Policy,
             class... MemberFunctionArgs>
        requires partially_callable<Result (Receiver::*)(MemberFunctionArgs...), Receiver, Args...>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect(
        Result (Receiver::*callable)(MemberFunctionArgs...),
        Receiver& guard,
        Policy&& policy) const -> connection<SharedPointer>
    {
        return connect_with_guard(member_function_lambda(callable, guard),
                                  guard,
                                  std::forward<Policy>(policy),
                                  false);
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<guard_like Receiver,
             class Result,
             execution_policy Policy,
             class... MemberFunctionArgs>
        requires partially_callable<Result (Receiver::*)(MemberFunctionArgs...), Receiver, Args...>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect_once(
        Result (Receiver::*callable)(MemberFunctionArgs...),
        Receiver& guard,
        Policy&& policy) const -> connection<SharedPointer>
    {
        return connect_with_guard(member_function_lambda(callable, guard),
                                  guard,
                                  std::forward<Policy>(policy),
                                  true);
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<guard_like Receiver,
             class Result,
             execution_policy Policy,
             class... MemberFunctionArgs>
        requires partially_callable<Result (Receiver::*)(MemberFunctionArgs...) const,
                                    const Receiver,
                                    Args...>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect(
        Result (Receiver::*callable)(MemberFunctionArgs...) const,
        const Receiver& guard,
        Policy&& policy) const -> connection<SharedPointer>
    {
        return connect_with_guard(member_function_lambda(callable, guard),
                                  guard,
                                  std::forward<Policy>(policy),
                                  false);
    }

    template<basic_lockable Mutex, template<class> class SharedPointer>
        requires shared_pointer_like<SharedPointer>
    template<signal_arg... Args>
    template<guard_like Receiver,
             class Result,
             execution_policy Policy,
             class... MemberFunctionArgs>
        requires partially_callable<Result (Receiver::*)(MemberFunctionArgs...) const,
                                    const Receiver,
                                    Args...>
    auto emitter<Mutex, SharedPointer>::signal<Args...>::connect_once(
        Result (Receiver::*callable)(MemberFunctionArgs...) const,
        const Receiver& guard,
        Policy&& policy) const -> connection<SharedPointer>
    {
        return connect_with_guard(member_function_lambda(callable, guard),
                                  guard,
                                  std::forward<Policy>(policy),
                                  true);
    }

    // ### connect class

    template<class Callable, execution_policy Policy>
    class connect_base
    {
    public:
        connect_base(Callable&& callable, Policy&& policy):
            m_callable { std::forward<Callable>(callable) },
            m_policy { std::forward<Policy>(policy) }
        {
        }

    protected:
        // It might be bad, but this is done on purpose.
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::decay_t<Callable> m_callable;
        // It might be bad, but this is done on purpose.
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::remove_reference_t<Policy> m_policy;
    };
} // namespace details

template<class Callable, details::execution_policy Policy>
class connect<Callable, void, Policy>: public details::connect_base<Callable, Policy>
{
public:
    explicit connect(Callable&& callable, Policy&& policy = {}):
        details::connect_base<Callable, Policy> { std::forward<Callable>(callable),
                                                  std::forward<Policy>(policy) }
    {
    }

    template<source_like Source>
    auto create_connection(Source&& origin) -> std::remove_cvref_t<Source>::connection_type
    {
        return std::forward<Source>(origin).connect(
            details::connect_base<Callable, Policy>::m_callable,
            details::connect_base<Callable, Policy>::m_policy);
    }
};

template<class Callable, guard_like Guard, details::execution_policy Policy>
class connect<Callable, Guard, Policy>: public details::connect_base<Callable, Policy>
{
public:
    connect(Callable&& callable, const Guard& guard, Policy&& policy = {}):
        details::connect_base<Callable, Policy> { std::forward<Callable>(callable),
                                                  std::forward<Policy>(policy) },
        m_guard { guard }
    {
    }

    template<source_like Source>
    auto create_connection(Source&& origin) -> std::remove_cvref_t<Source>::connection_type
    {
        return std::forward<Source>(origin).connect(
            details::connect_base<Callable, Policy>::m_callable,
            m_guard,
            details::connect_base<Callable, Policy>::m_policy);
    }

private:
    // It might be bad, but this is done on purpose.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const Guard& m_guard;
};

template<class Result, class... Args, guard_like Guard, details::execution_policy Policy>
class connect<Result (Guard::*)(Args...), Guard, Policy>
    : public details::connect_base<Result (Guard::*)(Args...), Policy>
{
public:
    connect(Result (Guard::*callable)(Args...), Guard& guard, Policy&& policy = {}):
        details::connect_base<Result (Guard::*)(Args...), Policy>(std::move(callable),
                                                                  std::forward<Policy>(policy)),
        m_guard { guard }
    {
    }

    template<source_like Source>
    auto create_connection(Source&& origin) -> std::remove_cvref_t<Source>::connection_type
    {
        return std::forward<Source>(origin).connect(
            details::connect_base<Result (Guard::*)(Args...), Policy>::m_callable,
            m_guard,
            details::connect_base<Result (Guard::*)(Args...), Policy>::m_policy);
    }

private:
    // It might be bad, but this is done on purpose.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    Guard& m_guard;
};

template<class Result, class... Args, guard_like Guard, details::execution_policy Policy>
class connect<Result (Guard::*)(Args...) const, const Guard, Policy>
    : public details::connect_base<Result (Guard::*)(Args...) const, Policy>
{
public:
    connect(Result (Guard::*callable)(Args...) const, const Guard& guard, Policy&& policy = {}):
        details::connect_base<Result (Guard::*)(Args...) const, Policy>(
            std::move(callable),
            std::forward<Policy>(policy)),
        m_guard { guard }
    {
    }

    template<source_like Source>
    auto create_connection(Source&& origin) -> std::remove_cvref_t<Source>::connection_type
    {
        return std::forward<Source>(origin).connect(
            details::connect_base<Result (Guard::*)(Args...) const, Policy>::m_callable,
            m_guard,
            details::connect_base<Result (Guard::*)(Args...) const, Policy>::m_policy);
    }

private:
    // It might be bad, but this is done on purpose.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const Guard& m_guard;
};

template<class Result, class... Args, guard_like Guard, details::execution_policy Policy>
connect(Result (Guard::*)(Args...) const, const Guard&, Policy)
    -> connect<Result (Guard::*)(Args...) const, Guard, Policy>;

template<class Result, class... Args, guard_like Guard>
connect(Result (Guard::*)(Args...) const, const Guard&)
    -> connect<Result (Guard::*)(Args...) const, Guard, details::synchronous_policy>;

template<class Result, class... Args, guard_like Guard, details::execution_policy Policy>
connect(Result (Guard::*)(Args...), Guard&, Policy)
    -> connect<Result (Guard::*)(Args...), Guard, Policy>;

template<class Result, class... Args, guard_like Guard>
connect(Result (Guard::*)(Args...), Guard&)
    -> connect<Result (Guard::*)(Args...), Guard, details::synchronous_policy>;

template<class Callable, guard_like Guard, details::execution_policy Policy>
connect(Callable&&, const Guard&, Policy&&) -> connect<Callable, Guard, Policy>;

template<class Callable, guard_like Guard>
connect(Callable&&, const Guard&) -> connect<Callable, Guard, details::synchronous_policy>;

template<class Callable, details::execution_policy Policy>
connect(Callable&&, Policy&&) -> connect<Callable, void, Policy>;

template<class Callable>
connect(Callable&&) -> connect<Callable, void, details::synchronous_policy>;

using basic_emitter = details::emitter<details::fake_mutex, details::unsafe_shared_pointer>;
using safe_emitter = details::emitter<std::mutex, std::shared_ptr>;
using basic_receiver = details::receiver<details::fake_mutex, details::unsafe_shared_pointer>;
using safe_receiver = details::receiver<std::mutex, std::shared_ptr>;

#endif // STIMULUS_H_