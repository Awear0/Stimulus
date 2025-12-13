#include "../sigslot.h"

#include <string>

#include <gtest/gtest.h>

#include "utilities.h"

class test_filter: public ::testing::Test
{
protected:
    generic_emitter<> empty_emitter;
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;

    static constexpr auto always_true { []<class... Args>(Args&&...) { return true; } };
    static constexpr auto always_false { []<class... Args>(Args&&...) { return false; } };
};

TEST_F(test_filter, always_true)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.apply(filter(always_true)).connect(slot_function<>);
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_filter, always_false)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.apply(filter(always_false)).connect(slot_function<>);
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 0);
}

TEST_F(test_filter, always_true_int)
{
    int& count = call_count<int>;
    reset<int>();

    int_emitter.generic_signal.apply(filter(always_true)).connect(slot_function<int>);
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

TEST_F(test_filter, always_false_int)
{
    int& count = call_count<int>;
    reset<int>();

    int_emitter.generic_signal.apply(filter(always_false)).connect(slot_function<int>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<int>.size(), 0);

    int_emitter.generic_emit(6);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<int>.size(), 0);
}

TEST_F(test_filter, only_even_int)
{
    static constexpr auto is_even { [](int value) { return (value % 2) == 0; } };

    int& count = call_count<int>;
    reset<int>();

    int_emitter.generic_signal.apply(filter(is_even)).connect(slot_function<int>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<int>.size(), 0);

    int_emitter.generic_emit(6);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 6);
}

TEST_F(test_filter, always_true_string)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    string_emitter.generic_signal.apply(filter(always_true)).connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    string_emitter.generic_emit("test");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "test");

    string_emitter.generic_emit("tset");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "tset");
}

TEST_F(test_filter, always_false_string)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    string_emitter.generic_signal.apply(filter(always_false)).connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    string_emitter.generic_emit("test");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    string_emitter.generic_emit("tset");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(call_args<std::string>.size(), 0);
}

TEST_F(test_filter, only_test_string)
{
    static constexpr auto only_test { [](std::string text) { return text == "test"; } };

    int& count = call_count<std::string>;
    reset<std::string>();

    string_emitter.generic_signal.apply(filter(only_test)).connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    string_emitter.generic_emit("test");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "test");

    string_emitter.generic_emit("tset");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "test");
}