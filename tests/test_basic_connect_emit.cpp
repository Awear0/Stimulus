#include "../sigslot.h"

#include <string>

#include <gtest/gtest.h>

#include "utilities.h"

class basic_connect_emit: public ::testing::Test
{
protected:
    generic_emitter<> empty_emitter;
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;
};

TEST_F(basic_connect_emit, void_emit)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_function<>);
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(basic_connect_emit, int_emit)
{
    int& count = call_count<int>;
    reset<int>();

    int_emitter.generic_signal.connect(slot_function<int>);
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

TEST_F(basic_connect_emit, string_emit)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    string_emitter.generic_signal.connect(slot_function<std::string>);
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

TEST_F(basic_connect_emit, int_string_emit)
{
    int& count = call_count<int, std::string>;
    reset<int, std::string>();

    int_string_emitter.generic_signal.connect(slot_function<int, std::string>);
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

TEST_F(basic_connect_emit, copy_move_emit)
{
    int& count = call_count<copy_move_counter>;
    reset<copy_move_counter>();

    copy_move_emitter.generic_signal.connect(slot_function<copy_move_counter>);
    EXPECT_EQ(count, 0);

    copy_move_emitter.generic_emit({});
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<copy_move_counter>.size(), 1);
    EXPECT_EQ(call_args<copy_move_counter>.back().copy_counter, 0);
    // 1 from generic_emit, 1 to pass it std::function, and 1 in the function itself.
    EXPECT_EQ(call_args<copy_move_counter>.back().move_counter, 3);
}

TEST_F(basic_connect_emit, 2_copy_move_emit)
{
    int& count = call_count<copy_move_counter>;
    reset<copy_move_counter>();

    copy_move_emitter.generic_signal.connect(slot_function<copy_move_counter>);
    copy_move_emitter.generic_signal.connect(slot_function<copy_move_counter>);
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

TEST_F(basic_connect_emit, lambda)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_lambda<>());
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}

TEST_F(basic_connect_emit, mutable_lambda)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_mutable_lambda<>());
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}

TEST_F(basic_connect_emit, functor)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_functor<>());
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}

TEST_F(basic_connect_emit, non_const_functor)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_non_const_functor<>());
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}

TEST_F(basic_connect_emit, emit_ref)
{
    int& count = call_count<int&>;
    count = 0;

    int ref { 0 };

    int_ref_emitter.generic_signal.connect([](int& value)
    {
        value = 45;
        count++;
    });
    EXPECT_EQ(count, 0);

    int_ref_emitter.generic_emit(ref);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(ref, 45);
}

TEST_F(basic_connect_emit, conversion)
{
    int& count = call_count<double>;
    reset<double>();

    int_emitter.generic_signal.connect(slot_function<double>);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(3);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<double>.size(), 1);
    EXPECT_EQ(call_args<double>.back(), 3.);
}