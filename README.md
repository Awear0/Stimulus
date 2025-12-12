# Overview

## Quick description

Sigslot is a C++ library implementing the observer pattern. It shares some concepts with Qt library, and reactive programming.


It is a C++26 only library, and has only been tested with Clang 21.1.5 so far.

Sigslot allows to add typed signal members to your own classes, and connect those signal to functions, lambdas, functors or method. Connections can be handled manually or automatically through scoped connections and guard classes instance.

## Simple examples

Sigslot allows to create Qt-style signal and connection:

```
#include <iostream>

// Inheriting emitter is required to access signals
class thermometer: public emitter
{
public:
    // Signal producing a double
    signal<double> temperature_changed;

    // Setter emitting previous signal
    void set_temperature(double value)
    {
        emit(&thermometer::temperature_changed, value);
    }
};

auto main() -> int
{
    thermometer t;
    // Connecting to a lambda
    auto connection = t.temperature_changed.connect([](double value)
    {
        std::cout << value << std::endl;
    });

    // Will print 5
    t.set_temperature(5.0);

    connection.disconnect();

    // Won't print anything after disconnection
    t.set_temperature(5.0);

    return 0;
}
```

It allows to create Qt-style slots as well:

```
#include <iostream>

class thermometer: public emitter
{
public:
    // Signal producing a double
    signal<double> temperature_changed;

    // Setter emitting previous signal
    void set_temperature(double value)
    {
        emit(&thermometer::temperature_changed, value);
    }
};

// Inheriting receiver allows to automatically disconnect signals connected to the class.
class temperature_printer: public receiver
{
public:
    // Slots are simply methods
    void print_temperature(double value)
    {
        std::cout << value << std::endl;
    }
};

auto main() -> int
{
    thermometer t;

    {
        temperature_printer p;

        // Connecting to a slot
        t.temperature_changed.connect(&temperature_printer::print_temperature, p);
        
        // Will print 5
        t.set_temperature(5.0);
    }

    // temperature_printer is destroyed here, so an automatic disconnection occurs.
    // Won't print anything
    t.set_temperature(5.0);

    return 0;
}
```

Finally, you can use the pipe operator to create reactive programming style connections:

```
#include <iostream>

class thermometer: public emitter
{
public:
    // Signal producing a double
    signal<double> temperature_changed;

    // Setter emitting previous signal
    void set_temperature(double value)
    {
        emit(&thermometer::temperature_changed, value);
    }
};

class temperature_printer: public receiver
{
public:
    // Slots are simply methods
    void print_temperature(std::string text)
    {
        std::cout << text << std::endl;
    }
};

double celsius_to_kelvin(double value)
{
    return value + 273.15;
}

template<class T>
struct greater_than
{
    greater_than(T value):
        m_value { value }
    {

    }

    auto operator()(T value) const -> bool
    {
        return value > m_value;
    }

    T m_value;
};

auto main() -> int
{
    double threshold { 50.0 };

    auto to_string
    {
        [](double value)
        {
            return std::to_string(value);
        }
    };

    thermometer t;

    temperature_printer p;

    // Connecting to a slot, after transforming the signal
    t.temperature_changed
        // Only emit if temperature is greater than threshold
        // The functor greater_than is used here for readability, but could be replaced by a lambda
        | filter(greater_than(threshold))
        // Convert to Kelvin
        | transform(celsius_to_kelvin)
        // Make it a string
        | transform(to_string)
        // Print it!
        | connect(&temperature_printer::print_temperature, p);
    
    // Doesn't print anything: it's not greater than threshold!
    t.set_temperature(5.0);

    // Print 373.15!
    t.set_temperature(100.0);

    return 0;
}
```

Many more features are available in Sigslot, such as suspend/resume on signals, signal forwarding, custom execution policies for slot execution in event loops, custom transformations, as well as thread-safety!

# Features

## Signals

Signals are the main features of Sigslot. In order for a class to have signal members, it must inherit `emitter`.

```
class my_class: public emitter
{
};
```

Each signal is associated to a list of parameters, that are emitted along with the signal itself. These parameters can be of any type or lvalue reference; they cannot be rvalue references. A signal can have an empty list of parameters.

**Users must be careful when using lvalue references as signal parameters** and must ensure that the referenced objects remain valid until all connected slots have been invoked.

```
class my_class: public emitter
{
public:
    // Empty signal
    signal<> empty_signal;
    // Int signal
    signal<int> int_signal;
    // Any class works as well
    signal<std::string> string_signal;
    // Signals support multiple parameters
    signal<int, double, char, std::string> many_parameters_signal;
    // References work too. Ensure reference validity throughout emission.
    signal<int&> reference_signal;
};
```

Signals can be emitted only from the class they're declared from, using the following syntax:

```
class my_class: public emitter
{
public:
    signal<int, double, char, std::string> many_parameters_signal;

    void function_that_emits()
    {
        /* This syntax might seem more cumbersome than required, but allows to ensure at
         * compile time that the signal belongs to the emitting class. */
        emit(&my_class::many_parameters_signal, 5, 3.14, 'c', "a string");
    }
};
```

### Const correctness

It must be noted that signals can be connected to and emitted even from a const class.

### Copy and move semantics

Signals have default copy/move constructor/assignment operators. The copy of a signal (and a moved signal) do not have any connections attached to it, even if the original signal had any.

This is made to ensure that `emitter` inheriting classes can still have default constructors/assignment operators.

## Connections

Connections allow to associate a signal to any number of slots (any Callable object, as per defined by the named requirement: https://en.cppreference.com/w/cpp/named_req/Callable.html).

Connections are created through the following syntax:

```
class dummy
{
public:
    int member;
    void method()
    {

    }
};

class my_class: public emitter
{
public:
    signal<int> int_signal;
    signal<dummy> dummy_signal;
    ...
};

void function(int) 
{

}

struct functor
{
    void operator()(int)
    {

    }
};

auto main() -> int
{
    my_class c;

    // Function
    c.int_signal.connect(function);
    // Lambda
    c.int_signal.connect([](int){});
    // Functor
    c.int_signal.connect(functor{});

    // Member function. This will call emitted_dummy.method().
    c.dummy_signal.connect(&dummy::method);
    /* Member. This will call emitted_dummy.member, which is arguably useless, since the
     * return value of a slot cannot be used. */
    c.dummy_signal.connect(&dummy::member);

    return 0;
}
```

Multiple connections can be made on a single signal.

### Connections handling

#### connection class

A call to the connect method will create a connection object.

```
connection conn = c.int_signal.connect(function);
// or
auto conn = c.int_signal.connect(function);
```

Several methods are available on the connection class:

##### disconnect

A call to `disconnect` will render the existing connection invalid, meaning that emission of the
signal won't trigger the call of the connected slot anymore.
This cannot be undone.

```
conn.disconnect();
```

##### suspend

This will move the connection to a suspended state. As long as a connection is suspended, any emission of the signal will be ignored.

```
conn.suspend();
```

##### resume

A call to resume will move the connection to a normal state, therefore cancelling the suspended state: emissions will now properly trigger the associated slot.

```
conn.resume();
```

##### add_exception_handler

If a slot called throws an exception, this exception will be propagated by default. One (or many) exception handlers can be installed on a connection. Those handlers will be called with the current exception ; if an exception is handled by at least one exception handler, it won't be propagated.
**One must ensure that exception handlers won't throw. If they do, this exception will be propagated to the calling context.**

```
void exception_handler(std::exception_ptr exception)
{
    try
    {
        std::rethrow_exception(exception);
    }
    catch(std::range_error& e)
    {
        ...
    }
    catch(std::overflow_error& e)
    {
        ...
    }
    catch(...)
    {
        ...
    }
}

conn.add_exception_handler(exception_handler);
```

When a slot throws, all exceptions handler registered to the connection are called in the order they have been registered. There is no way to remove a registered exception handler.

##### Copy and move semantics

Connection can safely be copy/move constructed/assigned. All copies of a connection share the same underlying state, including their connected/disconnected status, suspended/resumed state, and registered exception handlers.

#### scoped_connection class

A scoped_connection can be created from a connection object. A scoped connection will disconnect the connection when destructed.

```
{
    scoped_connection conn = c.int_signal.connect(function);
    // The connection is automatically disconnected when conn goes out of scope
}
```

Manual disconnect can be perfomed on a scoped connection:
```
{
    scoped_connection conn = c.int_signal.connect(function);
    conn.disconnect();
}
```

##### Copy and move semantics

A scoped_connection is move-only and cannot be copied.

#### inhibitor class

An inhibitor can be created from a connection object. Upon creation, the inhibitor will suspend the connection. The connection will be automatically resumed upon the inhibitor destruction.

```
{
    auto conn = c.int_signal.connect(function);
    inhibitor inh { conn };
    // The connection is suspended for now, any emission will be ignored.
    ...
    // The connection is automatically resumed when inh goes out of scope.
}
```

### Slot return value

Slots are not required to return void, but any return value will be ignored. There is no way for a signal to get the results of the slots it's connected to.

### Thread safety

`connect`, `disconnect`, `add_exception_handler`, `suspend`, `resume` and `emit` functions are thread safe, even when called on the same connection or signal.

### Slot call policy

By default, all slots are called synchronously. Custom execution policy for slots can be specified (see: Custom execution policy section).

### Partial argument match

When connecting a slot to a signal, the connection is valid if the slot can be called with the parameters of the signal, **or can be called by dropping any number of parameters, starting from the end**.

```
class my_class: public emitter
{
public:
    signal<int, std::string> int_signal;
};

void int_string_function(int, std::string)
{

}

void int_function(int) 
{

}

void empty_function()
{

}

auto main() -> int
{
    my_class c;

    // Valid
    c.int_signal.connect(int_string_function);
    // Valid as well, std::string argument will be dropped before calling int_function
    c.int_signal.connect(int_function);
    // Valid as well, all arguments will be dropped before calling int_function
    c.int_signal.connect(empty_function);

    return 0;
}
```

One must note that if multiple calls are valid (with multiple number of parameters), the one with the highest count of parameters will be picked.

### Argument conversion

Signal argument will be converted when possible before calling a slot:

```
class my_class: public emitter
{
public:
    signal<int> int_signal;
};

void double_function(double) 
{

}

auto main() -> int
{
    my_class c;

    // Valid: int to double conversion will be performed.
    c.int_signal.connect(double_function);

    return 0;
}
```

#### Value to reference and reference to value conversion

If a signal emits a reference, it can be connected to a slot taking a reference, or a value (a copy will be perfomed in the later case).

If a signal emits a value, it cannot be connected to a slot taking a reference. It can however be connected to a slot taking a const reference, or even an rvalue reference.

```
class my_class: public emitter
{
public:
    signal<int> int_signal;
    signal<int&> ref_signal;
};

void value_function(int value)
{

}

void ref_function(int& value)
{

}

void const_ref_function(const int& value)
{

}

void rvalue_function(int&& value)
{

}

auto main() -> int
{
    my_class c;

    // Valid
    c.int_signal.connect(value_function);

    // Invalid
    c.int_signal.connect(ref_function);

    // Valid
    c.int_signal.connect(const_ref_function);

    // Valid
    c.int_signal.connect(rvalue_function);

    // Valid
    c.ref_signal.connect(value_function);

    // Valid
    c.ref_signal.connect(ref_function);

    // Valid
    c.ref_signal.connect(const_ref_function);

    // Invalid
    c.ref_signal.connect(rvalue_function);

    return 0;
}
```

### Connection after signal destruction

Connections are automatically disconnected when their source signal is destroyed.

## Emitting

In order to call the slots connected to a signal, this signal must be emitted; signal parameters must be provided with each signal emission.
Only the class owning the signal is allowed to emit it.

During an emission, slots are called in the order they were registered.

To emit a signal, one must use the following syntax:

```
class my_class: public emitter
{
public:
    signal<int> int_signal;

    void emitting_function()
    {
        emit(&my_class::int_signal, 5);
    }
};
```

First, the signal must be passed to the emit function (in the form of a pointer to member parameter), then all the signal parameters.

## Signal forwarding

It is possible to connect a signal to another signal. In that case, the emission of the first signal will trigger the emission of the second one.
The source signal parameters must match the destination signal parameters, potentially after applying partial parameter dropping and type conversions.

This must be done using the following syntax:

```
class my_class: public emitter
{
public:
    signal<int> int_signal1;
    signal<int> int_signal2;
    signal<double> double_signal;
    signal<> empty_signal;

    my_class()
    {
        // Int to int, valid
        connect(int_signal1, &my_class::int_signal2);

        // Int to double, valid (a conversion is performed)
        connect(int_signal1, &my_class::double_signal);

        // Into nothing, valid (the int parameter is dropped).
        connect(int_signal1, &my_class::empty_signal);
    }
};
```

Only the class owning the signal is able to forward another signal to it.
Connection is disconnected whenever the object owning the destination signal is destructed.

## Guarding slots

In order to ensure that connections are automatically disconnected (and thus avoid dangling references), a guard mechanism exists.

This mechanism is implemented through the `receiver` class, which can be inherited from.
Any receiver inheriting object can be passed as a parameter of a connect call and act as a guard: the created connection will be automatically disconnected upon the guard destruction.

```
class my_emitter: public emitter
{
public:
    signal<int> int_signal;

    void emit_signal()
    {
        emit(&my_emitter::int_signal, 5);
    }
};

class my_receiver: public receiver
{
};

auto main() -> int
{
    my_emitter e;

    {
        my_receiver r;

        // Empty slot connection, with r as guard
        e.int_signal.connect([]{}, r);

        // Slot called
        e.emit_signal();
    }

    // Our guard r is now destroyed, the connection isn't valid anymore, nothing happens.
    e.emit_signal();

    return 0;
}
```

Methods of a guard can also be used as slots, effectively calling the method on the guard object when the slot is called.

```
class my_emitter: public emitter
{
public:
    signal<int> int_signal;

    void emit_signal()
    {
        emit(&my_emitter::int_signal, 5);
    }
};

class my_receiver: public receiver
{
public:
    void empty_function()
    {

    }
};

auto main() -> int
{
    my_emitter e;

    {
        my_receiver r;

        // Empty slot connection, with r as guard
        e.int_signal.connect(&my_receiver::empty_function, r);

        // Slot called
        e.emit_signal();
    }

    // Our guard r is now destroyed, the connection isn't valid anymore, nothing happens.
    e.emit_signal();

    return 0;
}
```

### Copy and move semantics

The `receiver` class has default copy/move constructor/assignment operator. Those methods are empty, and are just here to allow default copy/move operation on `receiver` inheriting classes. If a guard is destroyed after being copied/moved from, all the connections it's guarding will be invalid. A copy of a guard won't guard anything by default.

## Signal transformation

Transformations can be applied to signal before connecting them to a slot. Transformations can have different effects : filtering, applying function to parameters, parameters re-ordering...

In order to apply a transformation before connecting a slot to it, one must use the following syntax:

```
my_signal.apply(my_transformation).connect(my_slot);
```

Parameters convertion and partial parameter dropping can still be applied to slot connected to a transformed signal.

### Pipe operator

Instead of the apply method, the pipe operator can be used for more expressive code:

```
my_signal | my_transformation | connect(my_slot);
```

### Available transformations

#### map

map allows to re-order and drop parameters from a signal. map is a template class, templated over integers.
The template parameters count is the parameters count of the resulting signal. Each value represent a 0-based index of the original signal.
Eg: if you have a `signal<int, std::string>`, using map<1, 0> will generate the equivalent of a `signal<std::string, int>` (1 is the second parameter of the source: std::string, and 0 is the first parameter of the source: int). Parameters can be dropped that way (ex: `signal<int, std::string>` with map<1> will produce the equivalent of a `signal<std::string>`).
Parameters cannot be duplicated that way (map<0, 0> is invalid, because 0 appears twice), and values must be in range of the count of the source signal parameter (ex: map<2> would be invalid, and won't compile, with a `signal<int, char>`).

```
class my_emitter: public emitter
{
public:
    signal<int, std::string> int_string_signal;

    void emit_signal()
    {
        emit(&my_emitter::int_string_signal, 5, "test");
    }
};

void print(std::string text)
{
    std::cout << text << std::endl;
}

auto main() -> int
{
    my_emitter e;

    e.int_string_signal.apply(map<1>{}).connect(print);
    // This works as well!
    e.int_string_signal | map<1>{} | connect(print);

    // Slot with no parameter work too!
    e.int_string_signal | map<1>{} | connect([]{});

    return 0;
}
```

#### transform

transform allow to apply a function (transformation) to each parameter of the source signal before passing it to the slot. The transformation can modify the parameter type, as long as the slot can be called with the resulting type.
If fewer transformations are passed than the amount of parameters in the source signal, the remaining parameters will be forwarded as is.
`std::identity` can be used as a transformation that doesn't modify the parameter.

```
class my_emitter: public emitter
{
public:
    signal<int, std::string> int_string_signal;

    void emit_signal()
    {
        emit(&my_emitter::int_string_signal, 5, "test");
    }
};

auto to_string(int value) -> std::string
{
    return to_string(value);
}

auto to_int(std::string text) -> int
{
    return 42;
}

void print(std::string text)
{
    std::cout << text << std::endl;
}

auto main() -> int
{
    my_emitter e;

    // Creating a signal<std::string, int>.
    e.int_string_signal.apply(transform{ to_string, to_int }).connect(print);
    // This works as well!
    e.int_string_signal | transform{ to_string, to_int } | connect(print);

    // Fewer transformation than parameters (creating a signal<std::string, std::string>)
    e.int_string_signal | transform{ to_string } | connect(print);

    // Using std::identity
    e.int_string_signal | transform{ std::identity{}, to_int } | connect([](int, int){});

    return 0;
}
```

#### filter

filter allows to call a slot only if a predicate is respected. The source signal parameters must match the predicate signal parameters, potentially after applying partial parameter dropping and type conversions. The resulting call must be convertible to a `bool`

```
class my_emitter: public emitter
{
public:
    signal<int, std::string> int_string_signal;

    void emit_signal()
    {
        emit(&my_emitter::int_string_signal, 5, "test");
    }
};

auto main() -> int
{
    my_emitter e;

    e.int_string_signal.apply(filter([](int value, std::string text)
    {
        return value > 4 && text == "test";
    })).connect(print);

    // Works as well!
    e.int_string_signal | filter([](int value)
    {
        return value > 3;
    }) | connect(print);

    return 0;
}
```

### Custom transformations

Custom transformations can be implemented.
They must follow this pattern:

```
class my_transformation: public chainable
{
public:
    template<source_like Source>
    auto accept(Source&& origin) -> transformation_result<Source>
    {
        return { std::forward<Source>(origin) };
    }
};
```

and the resulting type of the accept call must follow this pattern:

```
template<source_like Source>
class transformation_result: public source,
                        public connectable
{
public:
    friend connectable;

    using args =
        typename std::remove_cvref_t<Source>::args;

    transformation_result(Source&& origin):
        m_source { std::forward<Source>(origin) }
    {
    }

private:
    template<partially_tuple_callable<
        typename std::remove_cvref_t<Source>::args> Callable>
    auto forwarding_lambda(Callable&& callable) const
    {
        return [callable = std::forward<Callable>(callable)]<class... Args>(Args&&... args) mutable
        {
            partial_call(callable, args...);
        };
    }

    Source m_source;
};
```

The resulting type must:
    - inherits source and connectable;
    - have an args public alias that is a tuple of the resulting signal parameters
    - have a method accessible to connectable named `forwarding lambda` that must take a Callable appliable to the transformed signal, and return a Callable accepting the input signal parameters, and applying the transformation of the signal.

Here's an example of a custom transformation that takes a signal, and return a signal that only has one parameter: the last parameter of the input signal.

```
class only_last_parameter: public chainable
{
public:
    template<source_like Source>
    auto accept(Source&& origin) -> only_last_parameter_result<Source>
    {
        return { std::forward<Source>(origin) };
    }
};
```

```
template<source_like Source>
    // Ensures there is at least on parameter in the input signal
    requires (std::tuple_size_v<typename std::remove_cvref_t<Source>::args> > 0)
class only_last_parameter_result: public source,
                        public connectable
{
public:
    friend connectable;

    // Alias to simplify code. Represent the input signal parameters.
    using input_tuple = typename std::remove_cvref_t<Source>::args;

    // Resulting parameters: only the last element of the input parameters.
    using args =
        std::tuple<std::tuple_element_t<std::tuple_size_v<input_tuple> - 1, input_tuple>>;

    only_last_parameter_result(Source&& origin):
        m_source { std::forward<Source>(origin) }
    {
    }

private:
    // forwarding_lambda
    template<partially_tuple_callable<args> Callable>
    auto forwarding_lambda(Callable&& callable) const
    {
        return [callable = std::forward<Callable>(callable)]<class... Args>(Args&&... args) mutable
        {
            // Only calls Callable with the last parameter.
            partial_call(callable, std::forward<Args...[sizeof...(Args) - 1]>(args...[sizeof...(Args) - 1]));
        };
    }

    Source m_source;
};
```

### Transformation chaining

Transformations can be chained, by either chaining `apply()` calls, or chaining pipe operator calls.

```
#include <iostream>

class thermometer: public emitter
{
public:
    // Signal producing a double
    signal<double> temperature_changed;

    // Setter emitting previous signal
    void set_temperature(double value)
    {
        emit(&thermometer::temperature_changed, value);
    }
};

class temperature_printer: public receiver
{
public:
    // Slots are simply methods
    void print_temperature(std::string text)
    {
        std::cout << text << std::endl;
    }
};

double celsius_to_kelvin(double value)
{
    return value + 273.15;
}

template<class T>
struct greater_than
{
    greater_than(T value):
        m_value { value }
    {

    }

    auto operator()(T value) const -> bool
    {
        return value > m_value;
    }

    T m_value;
};

auto main() -> int
{
    double threshold { 50.0 };

    auto to_string
    {
        [](double value)
        {
            return std::to_string(value);
        }
    };

    thermometer t;

    temperature_printer p;

    // Connecting to a slot, after transforming the signal
    t.temperature_changed
        // Only emit if temperature is greater than threshold
        // The functor greater_than is used here for readability, but could be replaced by a lambda
        | filter(greater_than(threshold))
        // Convert to Kelvin
        | transform(celsius_to_kelvin)
        // Make it a string
        | transform(to_string)
        // Print it!
        | connect(&temperature_printer::print_temperature, p);
    
    // Doesn't print anything: it's not greater than threshold!
    t.set_temperature(5.0);

    // Print 373.15!
    t.set_temperature(100.0);

    return 0;
}
```

Chains can be saved and applied to multiple signals:

```
#include <iostream>

class thermometer: public emitter
{
public:
    // Signal producing a double
    signal<double> temperature_changed;

    // Setter emitting previous signal
    void set_temperature(double value)
    {
        emit(&thermometer::temperature_changed, value);
    }
};

class temperature_printer: public receiver
{
public:
    // Slots are simply methods
    void print_temperature(std::string text)
    {
        std::cout << text << std::endl;
    }
};

double celsius_to_kelvin(double value)
{
    return value + 273.15;
}

template<class T>
struct greater_than
{
    greater_than(T value):
        m_value { value }
    {

    }

    auto operator()(T value) const -> bool
    {
        return value > m_value;
    }

    T m_value;
};

auto main() -> int
{
    double threshold { 50.0 };

    auto to_string
    {
        [](double value)
        {
            return std::to_string(value);
        }
    };

    thermometer t;
    thermometer t2;

    temperature_printer p;

    auto chain 
    {
        // Only emit if temperature is greater than threshold
        // The functor greater_than is used here for readability, but could be replaced by a lambda
        filter(greater_than(threshold))
        // Convert to Kelvin
        | transform(celsius_to_kelvin)
        // Make it a string
        | transform(to_string)
        // Print it!
        | connect(&temperature_printer::print_temperature, p);
    };

    t.temperature_changed
        | chain;

    t2.temperature_changed
        | chain;
    
    return 0;
}
```

## Custom execution policy

By default, all connections are executed synchronously. However, custom execution policy can be implemented.
To define a custom policy, a custom class must be created, containing the following features:
    - a method `void execute(std::function<void()>)`: This method will be called each time a slot should be executed. The parameter is a std::function that, when called, will execute the slot with the appropriate parameters. This function can be executed directly, stored, or even executed on another thread.
    - a static constexpr bool member named `is_synchronous`: The purpose of this member is to indicate whether the policy is synchronous or not. If the policy is synchronous, some optimization regarding parameters copy will be performed. If you're unsure, set this member to false.

Exemple of a custom policy that simply stores all potential slot invocations, instead of calling them:

```
struct storing_policy
{
    void execute(std::function<void()> callable)
    {
        m_functions.emplace_back(std::move(callable));
    }

    static constexpr bool is_synchronous { false };

    std::vector<std::function<void()>> m_functions;
};
```

**With asynchronous policies, one must be careful about signal with reference parameters, and must ensure that all references will stay valid until each slot is executed.**