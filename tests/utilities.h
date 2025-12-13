#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <list>

#include "../sigslot.h"

struct copy_move_counter
{
    int copy_counter { 0 };
    int move_counter { 0 };

    copy_move_counter() = default;

    copy_move_counter(const copy_move_counter& other):
        copy_counter { other.copy_counter + 1 },
        move_counter { other.move_counter }
    {
    }

    copy_move_counter(copy_move_counter&& other):
        copy_counter { other.copy_counter },
        move_counter { other.move_counter + 1 }
    {
    }
};

template<details::not_void... Args>
class generic_emitter: public basic_emitter
{
public:
    signal<Args...> generic_signal;

    void generic_emit(Args... args)
    {
        emit(&generic_emitter::generic_signal, std::forward<Args>(args)...);
    }
};

template<details::not_void... Args>
class safe_generic_emitter: public safe_emitter
{
public:
    signal<Args...> generic_signal;

    void generic_emit(Args... args)
    {
        emit(&safe_generic_emitter::generic_signal, std::forward<Args>(args)...);
    }
};

template<class... Args>
int call_count { 0 };

template<class T>
std::list<T> call_args {};

template<class... Args>
void reset()
{
    call_count<Args...> = 0;
    (call_args<Args>.clear(), ...);
}

template<class... Args>
void slot_function(Args... args)
{
    ++call_count<Args...>;
    (call_args<Args>.emplace_back(std::move(args)), ...);
}

template<class... Args>
auto slot_lambda()
{
    return [](Args... args) { slot_function(std::move(args)...); };
}

template<class... Args>
auto slot_mutable_lambda()
{
    int i { 0 };
    return [i](Args... args) mutable
    {
        ++i;
        slot_function(std::move(args)...);
    };
}

template<class... Args>
auto slot_functor()
{
    struct functor
    {
        void operator()(Args... args) const
        {
            slot_function(std::move(args)...);
        }
    };

    return functor {};
}

template<class... Args>
auto slot_non_const_functor()
{
    struct non_const_functor
    {
        void operator()(Args... args)
        {
            ++i;
            slot_function(std::move(args)...);
        }

        int i;
    };

    return non_const_functor {};
}

#endif