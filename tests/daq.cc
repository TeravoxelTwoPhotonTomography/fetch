#include <gtest/gtest.h>
#include <NIDAQmx.h>

TEST(NIDAQmxTest,Integration)
{ TaskHandle task;
  int32 sts;
  EXPECT_EQ(0, sts = DAQmxCreateTask("test",&task));
  EXPECT_EQ(0, sts = DAQmxClearTask(task));
}
