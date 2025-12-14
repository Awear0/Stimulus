#include "../stimulus.h"

#include <gtest/gtest.h>

using ::testing::Test;

TEST(all_different, basic_test)
{
    EXPECT_EQ((details::all_different<0, 1, 2, 3, 4, 5>), true);
    EXPECT_EQ((details::all_different<0, 1, 2>), true);
    EXPECT_EQ((details::all_different<0, 1>), true);
    EXPECT_EQ((details::all_different<0, 4>), true);
    EXPECT_EQ((details::all_different<0, 4, 1>), true);
    EXPECT_EQ((details::all_different<0>), true);
    EXPECT_EQ((details::all_different<1>), true);
    EXPECT_EQ((details::all_different<>), true);
    EXPECT_EQ((details::all_different<1, 1>), false);
    EXPECT_EQ((details::all_different<1, 0, 1>), false);
    EXPECT_EQ((details::all_different<1, 0, 2, 3, 4, 5, 0>), false);
}