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

// Inheriting receiver allows to disconnect automatically signals connected to the class.
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
