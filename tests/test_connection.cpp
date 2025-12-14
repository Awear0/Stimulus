#include "../stimulus.h"

#include <gtest/gtest.h>

#include "utilities.h"

class test_connection: public ::testing::Test
{
protected:
    generic_emitter<> empty_emitter;
};

TEST_F(test_connection, basic_disconnection)
{
    int& count = call_count<>;
    reset<>();

    auto connection { empty_emitter.generic_signal.connect(slot_function<>) };
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    connection.disconnect();

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}

TEST_F(test_connection, doesnt_disconnect_when_going_out_of_scope)
{
    int& count = call_count<>;
    reset<>();

    {
        auto connection { empty_emitter.generic_signal.connect(slot_function<>) };
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_connection, scoped_connection)
{
    int& count = call_count<>;
    reset<>();

    {
        scoped_connection scoped_connection { empty_emitter.generic_signal.connect(
            slot_function<>) };
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}

TEST_F(test_connection, scoped_connection_manual_disconnection)
{
    int& count = call_count<>;
    reset<>();

    {
        scoped_connection scoped_connection { empty_emitter.generic_signal.connect(
            slot_function<>) };
        EXPECT_EQ(count, 0);

        scoped_connection.disconnect();

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 0);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 0);
}

TEST_F(test_connection, basic_suspend_resume)
{
    int& count = call_count<>;
    reset<>();

    auto connection { empty_emitter.generic_signal.connect(slot_function<>) };
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    connection.suspend();

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    connection.resume();

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 2);
}

TEST_F(test_connection, doesnt_resume_when_going_out_of_scope)
{
    int& count = call_count<>;
    reset<>();

    {
        auto connection { empty_emitter.generic_signal.connect(slot_function<>) };
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);

        connection.suspend();

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 1);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}

TEST_F(test_connection, basic_inhibitor)
{
    int& count = call_count<>;
    reset<>();

    {
        inhibitor connection { empty_emitter.generic_signal.connect(slot_function<>) };
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 0);

        empty_emitter.generic_emit();
        EXPECT_EQ(count, 0);
    }

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}