#include <gtest/gtest.h>
#include "digitizer.pb.h"

TEST(Protobuf,Integration)
{ fetch::cfg::device::Digitizer d;
  ASSERT_TRUE(d.IsInitialized());
}
