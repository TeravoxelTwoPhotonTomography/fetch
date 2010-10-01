#include <gtest/gtest.h>
#include <niscope.h>
#include "digitizer.pb.h"

TEST(NIScopeTest,Integration)
{ fetch::cfg::device::Digitizer d;
  ViSession vi;
  EXPECT_EQ(
      VI_SUCCESS,
      niScope_init(
        const_cast<ViRsrc>(d.name().c_str()),
        NISCOPE_VAL_TRUE,  // ID Query
        NISCOPE_VAL_FALSE, // Do not reset
        &vi));
  EXPECT_EQ(
      VI_SUCCESS,
      niScope_close(vi));
}
