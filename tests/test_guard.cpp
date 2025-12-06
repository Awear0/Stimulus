#include "../sigslot.h"

#include <string>

#include <gtest/gtest.h>
#include <utility>

#include "utilities.h"

class test_guard: public ::testing::Test
{
protected:
    generic_emitter<> empty_emitter;
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;

    template<class... Args>
    struct generic_receiver: public receiver
    {
        void slot(Args&&... args)
        {
            slot_function<Args...>(std::forward<Args>(args)...);
        }

        void const_slot(Args&&... args) const
        {
            slot_function<Args...>(std::forward<Args>(args)...);
        }
    };
};

TEST_F(test_guard, member_function)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal.connect(&generic_receiver<>::slot, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, const_member_function)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal.connect(&generic_receiver<>::const_slot, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, guard)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal.connect(slot_function<>, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, member_function_with_map)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal.apply(map<> {}).connect(&generic_receiver<>::slot, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, const_member_function_with_map)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal.apply(map<> {}).connect(&generic_receiver<>::const_slot, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, guard_with_map)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal.apply(map<> {}).connect(slot_function<>, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, member_function_with_pipe)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal | connect(&generic_receiver<>::slot, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, const_member_function_with_pipe)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal | connect(&generic_receiver<>::const_slot, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, guard_with_pipe)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal | connect(slot_function<>, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, member_function_with_map_with_pipe)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal | map<> {} | connect(&generic_receiver<>::slot, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, const_member_function_with_map_with_pipe)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal | map<> {} | connect(&generic_receiver<>::const_slot, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, guard_with_map_with_pipe)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        empty_emitter.generic_signal | map<> {} | connect(slot_function<>, rec);
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, member_function_with_chain)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        auto chain { map<> {} | connect(&generic_receiver<>::slot, rec) };

        empty_emitter.generic_signal | chain;
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, const_member_function_with_chain)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        auto chain { map<> {} | connect(&generic_receiver<>::const_slot, rec) };

        empty_emitter.generic_signal | chain;
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_guard, guard_with_chain)
{
    int& count = call_count<>;
    reset<>();

    {
        generic_receiver<> rec;

        auto chain { map<> {} | connect(slot_function<>, rec) };

        empty_emitter.generic_signal | chain;
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 2);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}