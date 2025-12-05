#include "../sigslot.h"

#include <string>

#include <gtest/gtest.h>

#include "utilities.h"

class test_pipe_operator: public ::testing::Test
{
protected:
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;

    static auto to_string(int value) -> std::string
    {
        return std::to_string(value);
    }

    static auto is_even(int value) -> bool
    {
        return (value % 2) == 0;
    }
};

TEST_F(test_pipe_operator, no_effect_int)
{
    int& count = call_count<int>;
    reset<int>();

    int_emitter.generic_signal | map<0> {} | connect(slot_function<int>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 5);

    int_emitter.generic_emit(6);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 6);
}

TEST_F(test_pipe_operator, int_to_string)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    int_emitter.generic_signal | map<0> {} | transform(to_string) |
        connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "5");

    int_emitter.generic_emit(6);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_pipe_operator, even_only_to_string)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    int_emitter.generic_signal | map<0> {} | filter(is_even) | transform(to_string) |
        connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_emitter.generic_emit(6);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_pipe_operator, int_string_even_only_to_string)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    int_string_emitter.generic_signal | map<1, 0> {} | map<1> {} | filter(is_even) |
        transform(to_string) | connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "test");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_string_emitter.generic_emit(6, "tset");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_pipe_operator, int_string_even_only_to_string2)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    int_string_emitter.generic_signal | filter(is_even) | transform(to_string) |
        connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "test");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_string_emitter.generic_emit(6, "tset");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}