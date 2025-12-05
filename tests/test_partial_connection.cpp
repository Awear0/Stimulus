#include "../sigslot.h"

#include <string>

#include <gtest/gtest.h>

#include "utilities.h"

class partial_connect: public ::testing::Test
{
protected:
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;

    template<signal_arg... Args>
    struct forwarding_emitter: public emitter
    {
        template<signal_arg... EmittingArgs>
        forwarding_emitter(const signal<EmittingArgs...>& emitting_signal)
        {
            connect(emitting_signal, &forwarding_emitter::m_signal);
        }

        signal<Args...> m_signal;
    };

    template<signal_arg... Args>
    struct forwarding_mapped_emitter: public emitter
    {
        template<signal_arg... EmittingArgs>
        forwarding_mapped_emitter(const signal<EmittingArgs...>& emitting_signal)
        {
            connect(emitting_signal.apply(map<1, 0>()), &forwarding_mapped_emitter::m_signal);
        }

        signal<Args...> m_signal;
    };
};

TEST_F(partial_connect, int_emit)
{
    int& count = call_count<>;
    int& int_count = call_count<int>;
    reset<>();
    reset<int>();

    int_emitter.generic_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(int_count, 0);

    int_emitter.generic_emit(1);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(int_count, 0);

    int_emitter.generic_emit(2);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(int_count, 0);
}

TEST_F(partial_connect, string_emit)
{
    int& count = call_count<>;
    int& count_string = call_count<std::string>;
    reset<>();
    reset<std::string>();

    string_emitter.generic_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(count_string, 0);

    string_emitter.generic_emit("first");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(count_string, 0);

    string_emitter.generic_emit("second");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(count_string, 0);
}

TEST_F(partial_connect, partial_chaining_int)
{
    int& count = call_count<>;
    int& count_int = call_count<int>;
    reset<>();
    reset<int>();

    forwarding_emitter<> forwarding { int_emitter.generic_signal };

    forwarding.m_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(count_int, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(count_int, 0);

    int_emitter.generic_emit(4);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(count_int, 0);
}

TEST_F(partial_connect, partial_chaining_string_int)
{
    int& count = call_count<>;
    int& count_int = call_count<int>;
    int& count_int_string = call_count<int, std::string>;
    reset<>();
    reset<int>();
    reset<int, std::string>();

    forwarding_emitter<int> forwarding { int_string_emitter.generic_signal };

    forwarding.m_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(count_int, 0);
    EXPECT_EQ(count_int_string, 0);

    int_string_emitter.generic_emit(5, "abc");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(count_int, 0);
    EXPECT_EQ(count_int_string, 0);

    int_string_emitter.generic_emit(4, "def");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(count_int, 0);
    EXPECT_EQ(count_int_string, 0);
}

TEST_F(partial_connect, partial_mapped_chaining_string_int)
{
    int& count_string = call_count<std::string>;
    reset<>();
    reset<std::string>();

    forwarding_mapped_emitter<std::string> forwarding { int_string_emitter.generic_signal };

    forwarding.m_signal.connect(slot_function<std::string>);
    EXPECT_EQ(count_string, 0);

    int_string_emitter.generic_emit(5, "abc");
    EXPECT_EQ(count_string, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "abc");

    int_string_emitter.generic_emit(4, "def");
    EXPECT_EQ(count_string, 2);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "def");
}