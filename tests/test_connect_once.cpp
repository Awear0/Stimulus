#include "stimulus.h"

#include <gtest/gtest.h>

#include "utilities.h"

class test_connect_once: public ::testing::Test
{
protected:
    generic_emitter<> empty_emitter;
};

TEST_F(test_connect_once, connect_once)
{
    int& count = call_count<>;
    reset<>();

    auto connection { empty_emitter.generic_signal.connect_once(slot_function<>) };
    EXPECT_EQ(count, 0);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);

    empty_emitter.generic_emit();
    EXPECT_EQ(count, 1);
}