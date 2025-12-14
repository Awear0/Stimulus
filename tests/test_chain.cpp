#include "../stimulus.h"

#include <string>

#include <gtest/gtest.h>

#include "utilities.h"

class test_chain: public ::testing::Test
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

TEST_F(test_chain, no_effect_int)
{
    int& count = call_count<int>;
    reset<int>();

    auto chain { map<0> {} | connect(slot_function<int>) };

    int_emitter.generic_signal | chain;
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

TEST_F(test_chain, int_to_string)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto chain { map<0> {} | transform(to_string) | connect(slot_function<std::string>) };

    int_emitter.generic_signal | chain;
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

TEST_F(test_chain, even_only_to_string)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto chain { map<0> {} | filter(is_even) | transform(to_string) |
                 connect(slot_function<std::string>) };

    int_emitter.generic_signal | chain;
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_emitter.generic_emit(6);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_chain, int_string_even_only_to_string)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto chain { map<1, 0> {} | map<1> {} | filter(is_even) | transform(to_string) |
                 connect(slot_function<std::string>) };
    int_string_emitter.generic_signal | chain;
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "test");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_string_emitter.generic_emit(6, "tset");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_chain, int_string_even_only_to_string2)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto chain { filter(is_even) | transform(to_string) | connect(slot_function<std::string>) };

    int_string_emitter.generic_signal | chain;
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "test");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_string_emitter.generic_emit(6, "tset");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_chain, no_effect_int_transformation)
{
    int& count = call_count<int>;
    reset<int>();

    auto transformation { map<0> {} };

    int_emitter.generic_signal | transformation | connect(slot_function<int>);
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

TEST_F(test_chain, int_to_string_transformation)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto transformation { map<0> {} | transform(to_string) };

    int_emitter.generic_signal | transformation | connect(slot_function<std::string>);
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

TEST_F(test_chain, even_only_to_string_transformation)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto transformation { map<0> {} | filter(is_even) | transform(to_string) };

    int_emitter.generic_signal | transformation | connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_emitter.generic_emit(6);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_chain, int_string_even_only_to_string_transformation)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto transformation { map<1, 0> {} | map<1> {} | filter(is_even) | transform(to_string) };
    int_string_emitter.generic_signal | transformation | connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "test");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_string_emitter.generic_emit(6, "tset");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_chain, int_string_even_only_to_string2_transformation)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto transformation { filter(is_even) | transform(to_string) };

    int_string_emitter.generic_signal | transformation | connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "test");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_string_emitter.generic_emit(6, "tset");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_chain, use_chain_twice)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto chain { map<0> {} | filter(is_even) | transform(to_string) |
                 connect(slot_function<std::string>) };

    int_emitter.generic_signal | chain;
    int_string_emitter.generic_signal | chain;
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_emitter.generic_emit(6);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");

    int_string_emitter.generic_emit(7, "test");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");

    int_string_emitter.generic_emit(8, "test");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "8");
}

TEST_F(test_chain, use_transformation_twice)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    auto transformation { map<0> {} | filter(is_even) | transform(to_string) };

    int_emitter.generic_signal | transformation | connect(slot_function<std::string>);
    int_string_emitter.generic_signal | transformation | connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_emitter.generic_emit(6);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");

    int_string_emitter.generic_emit(7, "test");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "6");

    int_string_emitter.generic_emit(8, "test");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "8");
}