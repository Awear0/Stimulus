# Sigslot library

## Quick description

Sigslot is a C++ library implementing the observer pattern. It shares some concepts with Qt library, and reactive programming.


It is a C++26 only library, and has only been tested with Clang 21.1.5 so far.

Sigslot allows to add typed signal members to your own classes, and connect those signal to functions, lambdas, functors or method. Connections can be handled manually or automatically through scoped connections and guard classes instance.

## Quick overview

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

## Features

### Signals

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

#### Const correctness

It must be noted that signals can be connected to and emitted even from a const class.

#### Copy and move semantics

Signals have default copy/move constructor/assignment operators. The copy of a signal (and a moved signal) do not have any connections attached to it, even if the original signal had any.

This is made to ensure that `emitter` inheriting classes can still have default constructors/assignment operators.

### Connections

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

#### Connections handling

##### connection class

A call to the connect method will create a connection object.

```
connection conn = c.int_signal.connect(function);
// or
auto conn = c.int_signal.connect(function);
```

Several methods are available on the connection class:

###### disconnect

A call to `disconnect` will render the existing connection invalid, meaning that emission of the
signal won't trigger the call of the connected slot anymore.
This cannot be undone.

```
conn.disconnect();
```

###### suspend

This will move the connection to a suspended state. As long as a connection is suspended, any emission of the signal will be ignored.

```
conn.suspend();
```

###### resume

A call to resume will move the connection to a normal state, therefore cancelling the suspended state: emissions will now properly trigger the associated slot.

```
conn.resume();
```

###### add_exception_handler

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

###### Copy and move semantics

Connection can safely be copy/move constructed/assigned. All copies of a connection share the same underlying state, including their connected/disconnected status, suspended/resumed state, and registered exception handlers.

##### scoped_connection class

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

###### Copy and move semantics

A scoped_connection is move-only and cannot be copied.

##### inhibitor class

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

#### Slot return value

Slots are not required to return void, but any return value will be ignored. There is no way for a signal to get the results of the slots it's connected to.

#### Thread safety

`connect`, `disconnect`, `add_exception_handler` and `emit` functions are thread safe, even when called on the same connection or signal.

#### Slot call policy

By default, all slots are called synchronously. Custom execution policy for slots can be specified (see: Custom execution policy section).

#### Partial argument match

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

#### Argument conversion

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

##### Value to reference and reference to value conversion

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

#### Connection after signal destruction

Connection are automatically disconnected when their source signal is destroyed.