#include <gtest/gtest.h>

TEST(ExampleTest,Success)
{
  EXPECT_EQ(1,1);
}

TEST(ExampleTest,Failure)
{
  EXPECT_NE(1,1);
}


