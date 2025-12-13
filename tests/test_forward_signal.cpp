#include "../sigslot.h"

#include <string>

#include <gtest/gtest.h>

#include "utilities.h"

namespace
{
    auto to_string { [](int value) { return std::to_string(value); } };
    auto to_int { [](std::string text) { return std::atoi(text.c_str()); } };
} // namespace

class test_forward_signal: public ::testing::Test
{
protected:
    generic_emitter<> empty_emitter;
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;

    template<details::signal_arg... Args>
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
            connect(emitting_signal.apply(map<> {}), &map_to_nothing_forwarding_emitter::m_signal);
        }

        signal<> m_signal;
    };

    template<class... Args>
    struct map_to_first_forwarding_emitter: public emitter
    {
        map_to_first_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.apply(map<0> {}), &map_to_first_forwarding_emitter::m_signal);
        }

        signal<Args...[0]> m_signal;
    };

    template<class... Args>
    struct map_to_second_forwarding_emitter: public emitter
    {
        map_to_second_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.apply(map<1> {}), &map_to_second_forwarding_emitter::m_signal);
        }

        signal<Args...[1]> m_signal;
    };

    template<class... Args>
    struct map_swap_forwarding_emitter: public emitter
    {
        map_swap_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.apply(map<1, 0> {}), &map_swap_forwarding_emitter::m_signal);
        }

        signal<Args...[1], Args...[0]> m_signal;
    };

    template<class... Args>
    struct no_transformation_forwarding_emitter: public emitter
    {
        no_transformation_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.apply(transform {}),
                    &no_transformation_forwarding_emitter::m_signal);
        }

        signal<Args...> m_signal;
    };

    template<class... Args>
    struct to_string_to_int_transformation_forwarding_emitter: public emitter
    {
        to_string_to_int_transformation_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.apply(transform(to_string, to_int)),
                    &to_string_to_int_transformation_forwarding_emitter::m_signal);
        }

        signal<std::string, int> m_signal;
    };

    template<class... Args>
    struct to_string_transformation_forwarding_emitter: public emitter
    {
        to_string_transformation_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.apply(transform(to_string)),
                    &to_string_transformation_forwarding_emitter::m_signal);
        }

        signal<std::string, std::string> m_signal;
    };

    template<class... Args>
    struct to_int_transformation_forwarding_emitter: public emitter
    {
        to_int_transformation_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.apply(transform(std::identity {}, to_int)),
                    &to_int_transformation_forwarding_emitter::m_signal);
        }

        signal<int, int> m_signal;
    };

    template<class... Args>
    struct to_int_transformation_forwarding_emitter2: public emitter
    {
        to_int_transformation_forwarding_emitter2(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.apply(map<1, 0>()).apply(transform(to_int)).apply(map<1, 0>()),
                    &to_int_transformation_forwarding_emitter2::m_signal);
        }

        signal<int, int> m_signal;
    };

    template<class... Args>
    struct double_swap_forwarding_emitter: public emitter
    {
        double_swap_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal.apply(map<1, 0>()).apply(transform(to_int, to_string)),
                    &double_swap_forwarding_emitter::m_signal);
        }

        signal<int, std::string> m_signal;
    };

    template<class... Args>
    struct pipe_forwarding_emitter: public emitter
    {
        pipe_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            connect(emitting_signal | map<0>(), &pipe_forwarding_emitter::m_signal);
        }

        signal<Args...[0]> m_signal;
    };

    template<class... Args>
    struct full_pipe_forwarding_emitter: public emitter
    {
        full_pipe_forwarding_emitter(const signal<Args...>& emitting_signal)
        {
            emitting_signal | map<0>() | forward_to(&full_pipe_forwarding_emitter::m_signal);
        }

        signal<Args...[0]> m_signal;
    };

    template<class... Args>
    struct signal_pipe_signal_emitter: public emitter
    {
        signal_pipe_signal_emitter(const signal<Args...>& emitting_signal)
        {
            emitting_signal | forward_to(&signal_pipe_signal_emitter::m_signal);
        }

        signal<Args...[0]> m_signal;
    };

    template<class... Args>
    struct signal_pipe_signal_chain_emitter: public emitter
    {
        signal_pipe_signal_chain_emitter(const signal<Args...>& emitting_signal)
        {
            auto chain { map<0>() | forward_to(&signal_pipe_signal_chain_emitter::m_signal) };
            emitting_signal | chain;
        }

        signal<Args...[0]> m_signal;
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

TEST_F(test_forward_signal, empty_transform_empty_signal)
{
    int& count = call_count<>;
    reset<>();

    no_transformation_forwarding_emitter receiver(empty_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_forward_signal, empty_transform_int_signal)
{
    int& count = call_count<int>;
    reset<int>();

    no_transformation_forwarding_emitter receiver(int_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<int>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(44);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 44);

    int_emitter.generic_emit(47);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 47);
}

TEST_F(test_forward_signal, empty_transform_int_string_signal)
{
    int& count = call_count<int, std::string>;
    reset<int, std::string>();

    no_transformation_forwarding_emitter receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<int, std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(44, "55");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 44);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "55");

    int_string_emitter.generic_emit(47, "57");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 47);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "57");
}

TEST_F(test_forward_signal, to_string_to_int_transform_int_string_signal)
{
    int& count = call_count<std::string, int>;
    reset<std::string, int>();

    to_string_to_int_transformation_forwarding_emitter receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<std::string, int>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(44, "55");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 55);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "44");

    int_string_emitter.generic_emit(47, "57");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 57);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "47");
}

TEST_F(test_forward_signal, to_string_transform_int_string_signal)
{
    int& count = call_count<std::string, std::string>;
    reset<std::string, std::string>();

    to_string_transformation_forwarding_emitter receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<std::string, std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(44, "55");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "55");
    EXPECT_EQ(*call_args<std::string>.begin(), "44");

    int_string_emitter.generic_emit(47, "57");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<std::string>.size(), 4);
    EXPECT_EQ(call_args<std::string>.back(), "57");
    EXPECT_EQ(*std::next(call_args<std::string>.begin(), 2), "47");
}

TEST_F(test_forward_signal, to_int_transform_int_string_signal)
{
    int& count = call_count<int, int>;
    reset<int, int>();

    to_int_transformation_forwarding_emitter receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<int, int>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(44, "55");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 55);
    EXPECT_EQ(*call_args<int>.begin(), 44);

    int_string_emitter.generic_emit(47, "57");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 4);
    EXPECT_EQ(call_args<int>.back(), 57);
    EXPECT_EQ(*std::next(call_args<int>.begin(), 2), 47);
}

TEST_F(test_forward_signal, to_int_transform_int_string_signal2)
{
    int& count = call_count<int, int>;
    reset<int, int>();

    to_int_transformation_forwarding_emitter2 receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<int, int>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(44, "55");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 55);
    EXPECT_EQ(*call_args<int>.begin(), 44);

    int_string_emitter.generic_emit(47, "57");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 4);
    EXPECT_EQ(call_args<int>.back(), 57);
    EXPECT_EQ(*std::next(call_args<int>.begin(), 2), 47);
}

TEST_F(test_forward_signal, double_swamp_forwarding)
{
    int& count = call_count<int, std::string>;
    reset<int, std::string>();

    double_swap_forwarding_emitter receiver(int_string_emitter.generic_signal);

    receiver.m_signal.connect(slot_function<int, std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(44, "55");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 55);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "44");

    int_string_emitter.generic_emit(47, "57");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 57);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "47");
}

TEST_F(test_forward_signal, disconnect_on_receiver_destruction)
{
    int& count = call_count<>;
    reset<>();

    {
        forwarding_emitter<> receiver(empty_emitter.generic_signal);
        receiver.m_signal.connect(slot_function<>);
    }

    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 0);
}

TEST_F(test_forward_signal, pipe_forwarding)
{
    int& count = call_count<int>;
    reset<int>();

    pipe_forwarding_emitter receiver(int_string_emitter.generic_signal);

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

TEST_F(test_forward_signal, full_pipe_forwarding)
{
    int& count = call_count<int>;
    reset<int>();

    full_pipe_forwarding_emitter receiver(int_string_emitter.generic_signal);

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

TEST_F(test_forward_signal, signal_pipe_signal)
{
    int& count = call_count<int>;
    reset<int>();

    signal_pipe_signal_emitter receiver(int_string_emitter.generic_signal);

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

TEST_F(test_forward_signal, signal_pipe_signal_chain)
{
    int& count = call_count<int>;
    reset<int>();

    signal_pipe_signal_chain_emitter receiver(int_string_emitter.generic_signal);

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