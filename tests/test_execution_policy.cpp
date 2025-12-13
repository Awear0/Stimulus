#include "../sigslot.h"

#include <functional>
#include <string>

#include <gtest/gtest.h>
#include <vector>

#include "utilities.h"

class custom_policy_connect_emit: public ::testing::Test
{
protected:
    generic_emitter<> empty_emitter;
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;

    struct storing_policy
    {
        void execute(std::function<void()> callable)
        {
            m_functions.emplace_back(std::move(callable));
        }

        static constexpr bool is_synchronous { false };

        std::vector<std::function<void()>> m_functions;
    };

    storing_policy policy;
};

TEST_F(custom_policy_connect_emit, void_emit)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_function<>, policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 2);

    policy.m_functions.front()();
    EXPECT_EQ(count, 1);

    policy.m_functions.back()();
    EXPECT_EQ(count, 2);
}

TEST_F(custom_policy_connect_emit, int_emit)
{
    int& count = call_count<int>;
    reset<int>();

    int_emitter.generic_signal.connect(slot_function<int>, policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    int_emitter.generic_emit(5);
    EXPECT_EQ(policy.m_functions.size(), 1);
    EXPECT_EQ(count, 0);

    int_emitter.generic_emit(6);
    EXPECT_EQ(policy.m_functions.size(), 2);
    EXPECT_EQ(count, 0);

    policy.m_functions.front()();
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 5);

    policy.m_functions.back()();
    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 6);
}

TEST_F(custom_policy_connect_emit, string_emit)
{
    int& count = call_count<std::string>;
    reset<std::string>();

    string_emitter.generic_signal.connect(slot_function<std::string>, policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    string_emitter.generic_emit("first");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 1);

    string_emitter.generic_emit("second");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 2);

    policy.m_functions.front()();

    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "first");

    policy.m_functions.back()();

    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "second");
}

TEST_F(custom_policy_connect_emit, int_string_emit)
{
    int& count = call_count<int, std::string>;
    reset<int, std::string>();

    int_string_emitter.generic_signal.connect(slot_function<int, std::string>, policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    int_string_emitter.generic_emit(5, "first");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 1);

    int_string_emitter.generic_emit(6, "second");
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 2);

    policy.m_functions.front()();

    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<int>.size(), 1);
    EXPECT_EQ(call_args<int>.back(), 5);
    EXPECT_EQ(call_args<std::string>.size(), 1);
    EXPECT_EQ(call_args<std::string>.back(), "first");

    policy.m_functions.back()();

    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<int>.size(), 2);
    EXPECT_EQ(call_args<int>.back(), 6);
    EXPECT_EQ(call_args<std::string>.size(), 2);
    EXPECT_EQ(call_args<std::string>.back(), "second");
}

TEST_F(custom_policy_connect_emit, copy_move_emit)
{
    int& count = call_count<copy_move_counter>;
    reset<copy_move_counter>();

    copy_move_emitter.generic_signal.connect(slot_function<copy_move_counter>, policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    copy_move_emitter.generic_emit({});
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 1);

    policy.m_functions.front()();

    EXPECT_EQ(call_args<copy_move_counter>.size(), 1);

    EXPECT_EQ(call_args<copy_move_counter>.back().copy_counter, 0);
    // 1 from generic_emit, 1 to pass it std::function, and 1 in the function itself, 1 from moving
    // std::function in policy, and 1 from moving to the policy
    EXPECT_EQ(call_args<copy_move_counter>.back().move_counter, 5);
}

TEST_F(custom_policy_connect_emit, 2_copy_move_emit)
{
    int& count = call_count<copy_move_counter>;
    reset<copy_move_counter>();

    copy_move_emitter.generic_signal.connect(slot_function<copy_move_counter>, policy);
    copy_move_emitter.generic_signal.connect(slot_function<copy_move_counter>, policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    copy_move_emitter.generic_emit({});
    EXPECT_EQ(policy.m_functions.size(), 2);

    policy.m_functions.front()();
    policy.m_functions.back()();

    EXPECT_EQ(count, 2);
    EXPECT_EQ(call_args<copy_move_counter>.size(), 2);
    // 1 from the execution policy
    EXPECT_EQ(call_args<copy_move_counter>.back().copy_counter, 0);
    // 1 from generic_emit, 1 to pass it std::function, and 1 in the function itself, 1 from moving
    // std::function in policy, and 1 from moving to the policy
    EXPECT_EQ(call_args<copy_move_counter>.back().move_counter, 5);
    // 1 to pass it to std::function
    EXPECT_EQ(call_args<copy_move_counter>.front().copy_counter, 1);
    // 1 from generic_emit, 1 to pass it std::function, and 1 in the function itself.
    EXPECT_EQ(call_args<copy_move_counter>.front().move_counter, 4);
}

TEST_F(custom_policy_connect_emit, lambda)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_lambda<>(), policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(policy.m_functions.size(), 1);

    policy.m_functions.front()();
    EXPECT_EQ(count, 1);
}

TEST_F(custom_policy_connect_emit, mutable_lambda)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_mutable_lambda<>(), policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(policy.m_functions.size(), 1);

    policy.m_functions.front()();
    EXPECT_EQ(count, 1);
}

TEST_F(custom_policy_connect_emit, functor)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_functor<>(), policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(policy.m_functions.size(), 1);

    policy.m_functions.front()();
    EXPECT_EQ(count, 1);
}

TEST_F(custom_policy_connect_emit, non_const_functor)
{
    int& count = call_count<>;
    reset<>();

    empty_emitter.generic_signal.connect(slot_non_const_functor<>(), policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(policy.m_functions.size(), 1);

    policy.m_functions.front()();
    EXPECT_EQ(count, 1);
}

TEST_F(custom_policy_connect_emit, emit_ref)
{
    int& count = call_count<int&>;
    count = 0;

    int ref { 0 };

    int_ref_emitter.generic_signal.connect(
        [](int& value)
    {
        value = 45;
        count++;
    },
        policy);

    EXPECT_EQ(policy.m_functions.size(), 0);
    EXPECT_EQ(count, 0);

    int_ref_emitter.generic_emit(ref);
    EXPECT_EQ(policy.m_functions.size(), 1);

    policy.m_functions.front()();
    EXPECT_EQ(count, 1);
    EXPECT_EQ(ref, 45);
}

TEST_F(custom_policy_connect_emit, conversion)
{
    int& count = call_count<double>;
    reset<double>();

    int_emitter.generic_signal.connect(slot_function<double>, policy);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(policy.m_functions.size(), 0);

    int_emitter.generic_emit(3);
    EXPECT_EQ(policy.m_functions.size(), 1);

    policy.m_functions.front()();
    EXPECT_EQ(count, 1);
    EXPECT_EQ(call_args<double>.size(), 1);
    EXPECT_EQ(call_args<double>.back(), 3.);
}