#include "../sigslot.h"

#include <string>

#include <gtest/gtest.h>

#include "utilities.h"

class test_forward_signal: public ::testing::Test
{
protected:
    generic_emitter<> empty_emitter;
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;

    template<signal_arg... Args>
    struct forwarding_emitter: public emitter
    {
        forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal, &forwarding_emitter::m_signal);
        }

        signal<Args...> m_signal;
    };

    struct map_to_nothing_forwarding_emitter: public emitter
    {
        template<class... Args>
        map_to_nothing_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.template map<>(), &map_to_nothing_forwarding_emitter::m_signal);
        }

        signal<> m_signal;
    };

    template<class... Args>
    struct map_to_first_forwarding_emitter: public emitter
    {
        map_to_first_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.template map<0>(), &map_to_first_forwarding_emitter::m_signal);
        }

        signal<Args...[0]> m_signal;
    };

    template<class... Args>
    struct map_to_second_forwarding_emitter: public emitter
    {
        map_to_second_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.template map<1>(), &map_to_second_forwarding_emitter::m_signal);
        }

        signal<Args...[1]> m_signal;
    };

    template<class... Args>
    struct map_swap_forwarding_emitter: public emitter
    {
        map_swap_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.template map<1, 0>(), &map_swap_forwarding_emitter::m_signal);
        }

        signal<Args...[1], Args...[0]> m_signal;
    };
};

TEST_F(test_forward_signal, same_empty_signal)
{
    int& count = call_count<>;
    reset<>();

    forwarding_emitter<> receiver(empty_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_forward_signal, same_empty_signal_connect_once)
{
    int& count = call_count<>;
    reset<>();

    forwarding_emitter<> receiver(empty_emitter.generic_signal);

    receiver.m_signal.connect_once(slot_function<>);
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}

TEST_F(test_forward_signal, same_int_signal)
{
    int& count = call_count<int>;
    reset<int>();

    forwarding_emitter<int> receiver(int_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<int>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(1);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 1);

    int_emitter.generic_emit(2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 2);
    EXPECT_EQ(count, 2);
}

TEST_F(test_forward_signal, same_string_signal)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    forwarding_emitter<std::string> receiver(string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    string_emitter.generic_emit("test1");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "test1");

    string_emitter.generic_emit("test2");
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "test2");
    EXPECT_EQ(count, 2);
}

TEST_F(test_forward_signal, same_int_string_signal)
{
    int& count = call_count<int, std::string>;
    reset<int, std::string>();

    forwarding_emitter<int, std::string> receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<int, std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "55");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 5);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "55");

    int_string_emitter.generic_emit(7, "77");
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 7);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "77");
    EXPECT_EQ(count, 2);
}

TEST_F(test_forward_signal, empty_map_empty_signal)
{
    int& count = call_count<>;
    reset<>();

    map_to_nothing_forwarding_emitter receiver(empty_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_forward_signal, empty_map_int_signal)
{
    int& count = call_count<>;
    reset<>();

    map_to_nothing_forwarding_emitter receiver(int_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(7);
    EXPECT_EQ(count, 1);

    int_emitter.generic_emit(8);
    EXPECT_EQ(count, 2);
}

TEST_F(test_forward_signal, empty_map_string_signal)
{
    int& count = call_count<>;
    reset<>();

    map_to_nothing_forwarding_emitter receiver(string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);

    string_emitter.generic_emit("7");
    EXPECT_EQ(count, 1);

    string_emitter.generic_emit("8");
    EXPECT_EQ(count, 2);
}

TEST_F(test_forward_signal, empty_map_int_string_signal)
{
    int& count = call_count<>;
    reset<>();

    map_to_nothing_forwarding_emitter receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(7, "7");
    EXPECT_EQ(count, 1);

    int_string_emitter.generic_emit(8, "8");
    EXPECT_EQ(count, 2);
}

TEST_F(test_forward_signal, int_map_int_string_signal)
{
    int& count = call_count<int>;
    reset<int>();

    map_to_first_forwarding_emitter receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<int>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(7, "7");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 7);

    int_string_emitter.generic_emit(8, "8");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 8);
}

TEST_F(test_forward_signal, string_map_int_string_signal)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    map_to_second_forwarding_emitter receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(7, "7");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "7");

    int_string_emitter.generic_emit(8, "8");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "8");
}

TEST_F(test_forward_signal, string_int_map_int_string_signal)
{
    int& count = call_count<std::string, int>;
    reset<std::string, int>();

    map_swap_forwarding_emitter receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<std::string, int>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(7, "7");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "7");
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 7);

    int_string_emitter.generic_emit(8, "8");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "8");
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 8);
}