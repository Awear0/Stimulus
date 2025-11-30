#include "../sigslot.h"

#include <cstdlib>
#include <functional>
#include <string>

#include <gtest/gtest.h>

#include "utilities.h"

class test_transform: public ::testing::Test
{
protected:
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;
};

TEST_F(test_transform, no_effect_int)
{
    int& count = call_count<int>;
    reset<int>();

    int_emitter.generic_signal.transform().connect(slot_function<int>);
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

TEST_F(test_transform, no_effect_string)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    string_emitter.generic_signal.transform().connect(slot_function<std::string>);
    EXPECT_EQ(count, 0);

    string_emitter.generic_emit("first");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "first");

    string_emitter.generic_emit("second");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "second");
}

TEST_F(test_transform, copy_move_no_effect)
{
    int& count = call_count<copy_move_counter>;
    reset<copy_move_counter>();

    copy_move_emitter.generic_signal.transform().connect(slot_function<copy_move_counter>);
    EXPECT_EQ(count, 0);

    copy_move_emitter.generic_emit({});
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<copy_move_counter>.size(), 1);
    EXPECT_EQ(call_args<copy_move_counter>.back().copy_counter, 0);
    // 1 from generic_emit, 1 to pass it std::function, and 1 in the function itself.
    EXPECT_EQ(call_args<copy_move_counter>.back().move_counter, 3);
}

TEST_F(test_transform, 2_copy_move_no_effect)
{
    int& count = call_count<copy_move_counter>;
    reset<copy_move_counter>();

    copy_move_emitter.generic_signal.transform().connect(slot_function<copy_move_counter>);
    copy_move_emitter.generic_signal.transform().connect(slot_function<copy_move_counter>);
    EXPECT_EQ(count, 0);

    copy_move_emitter.generic_emit({});
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<copy_move_counter>.size(), 2);
    EXPECT_EQ(call_args<copy_move_counter>.back().copy_counter, 0);
    // 1 from generic_emit, 1 to pass it std::function, and 1 in the function itself.
    EXPECT_EQ(call_args<copy_move_counter>.back().move_counter, 3);
    // 1 to pass it to std::function
    EXPECT_EQ(call_args<copy_move_counter>.front().copy_counter, 1);
    // 1 from generic_emit, 1 to pass it std::function, and 1 in the function itself.
    EXPECT_EQ(call_args<copy_move_counter>.front().move_counter, 2);
}

namespace
{
    auto to_string { [](int value) { return std::to_string(value); } };
    auto to_int { [](std::string text) { return std::atoi(text.c_str()); } };
} // namespace

TEST_F(test_transform, swap_int_string)
{
    int& count = call_count<std::string, int>;
    reset<std::string, int>();

    int_string_emitter.generic_signal.transform(to_string, to_int)
        .connect(slot_function<std::string, int>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "42");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 42);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "5");

    int_string_emitter.generic_emit(6, "55");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 55);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "6");
}

TEST_F(test_transform, int_string_to_int_int)
{
    int& count = call_count<int, int>;
    reset<int, int>();

    int_string_emitter.generic_signal.transform(std::identity {}, to_int)
        .connect(slot_function<int, int>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "55");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.front(), 5);
    EXPECT_EQ(call_args<int>.back(), 55);
    EXPECT_EQ(call_args<std::string>.size(), 0);

    int_string_emitter.generic_emit(6, "66");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 4);
    EXPECT_EQ(*std::next(call_args<int>.begin(), 2), 6);
    EXPECT_EQ(call_args<int>.back(), 66);
    EXPECT_EQ(call_args<std::string>.size(), 0);
}

TEST_F(test_transform, int_string_to_string_string)
{
    int& count = call_count<std::string, std::string>;
    reset<std::string, std::string>();

    int_string_emitter.generic_signal.transform(to_string).connect(
        slot_function<std::string, std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "first");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.front(), "5");
    EXPECT_EQ(call_args<std::string>.back(), "first");

    int_string_emitter.generic_emit(6, "second");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 4);
    EXPECT_EQ(*std::next(call_args<std::string>.begin(), 2), "6");
    EXPECT_EQ(call_args<std::string>.back(), "second");
}

TEST_F(test_transform, int_string_to_nothing)
{
    int& count = call_count<int, std::string>;
    reset<int, std::string>();

    int_string_emitter.generic_signal.transform().connect(slot_function<int, std::string>);
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "first");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 5);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "first");

    int_string_emitter.generic_emit(6, "second");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 6);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "second");
}

TEST_F(test_transform, int_string_to_string_string_lambda)
{
    int& count = call_count<std::string, std::string>;
    reset<std::string, std::string>();

    int_string_emitter.generic_signal.transform(to_string).connect(
        slot_lambda<std::string, std::string>());
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "first");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.front(), "5");
    EXPECT_EQ(call_args<std::string>.back(), "first");

    int_string_emitter.generic_emit(6, "second");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 4);
    EXPECT_EQ(*std::next(call_args<std::string>.begin(), 2), "6");
    EXPECT_EQ(call_args<std::string>.back(), "second");
}

TEST_F(test_transform, int_string_to_string_string_mutable_lambda)
{
    int& count = call_count<std::string, std::string>;
    reset<std::string, std::string>();

    int_string_emitter.generic_signal.transform(to_string).connect(
        slot_mutable_lambda<std::string, std::string>());
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "first");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.front(), "5");
    EXPECT_EQ(call_args<std::string>.back(), "first");

    int_string_emitter.generic_emit(6, "second");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 4);
    EXPECT_EQ(*std::next(call_args<std::string>.begin(), 2), "6");
    EXPECT_EQ(call_args<std::string>.back(), "second");
}

TEST_F(test_transform, int_string_to_string_string_functor)
{
    int& count = call_count<std::string, std::string>;
    reset<std::string, std::string>();

    int_string_emitter.generic_signal.transform(to_string).connect(
        slot_functor<std::string, std::string>());
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "first");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.front(), "5");
    EXPECT_EQ(call_args<std::string>.back(), "first");

    int_string_emitter.generic_emit(6, "second");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 4);
    EXPECT_EQ(*std::next(call_args<std::string>.begin(), 2), "6");
    EXPECT_EQ(call_args<std::string>.back(), "second");
}

TEST_F(test_transform, int_string_to_string_string_non_const_functor)
{
    int& count = call_count<std::string, std::string>;
    reset<std::string, std::string>();

    int_string_emitter.generic_signal.transform(to_string).connect(
        slot_non_const_functor<std::string, std::string>());
    EXPECT_EQ(count, 0);

    int_string_emitter.generic_emit(5, "first");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.front(), "5");
    EXPECT_EQ(call_args<std::string>.back(), "first");

    int_string_emitter.generic_emit(6, "second");
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 0);
    EXPECT_EQ(call_args<std::string>.size(), 4);
    EXPECT_EQ(*std::next(call_args<std::string>.begin(), 2), "6");
    EXPECT_EQ(call_args<std::string>.back(), "second");
}