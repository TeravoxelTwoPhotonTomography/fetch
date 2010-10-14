#include <gtest/gtest.h>
#include "devices/microscope.h"
#include "file.pb.h"
#include <gtest/gtest.h>

/*
 * TODO: Add tests that check for the presence of generated files and 
 *       directories
 */

class FileSeriesFixture: public ::testing::Test
{
  public:
    fetch::cfg::FileSeries    config;
    fetch::device::FileSeries *self;

    virtual void SetUp()
    {
      config.set_root(".");
      self = new fetch::device::FileSeries(&config);
    }
    virtual void TearDown()
    {
      delete self;
    }
};

TEST_F(FileSeriesFixture,EnsurePathExistsWithBadRootFails)
{
  self->_desc->set_root("should_not_exist");
  EXPECT_DEATH(self->ensurePathExists(),"");
}  

TEST_F(FileSeriesFixture,EnsurePathExistsWithGoodRootSucceeds)
{
  self->ensurePathExists();
  SUCCEED();
}  

TEST_F(FileSeriesFixture,MultipleEnsurePathExistsWithGoodRootSucceeds)
{
  self->ensurePathExists();
  self->ensurePathExists();
  self->ensurePathExists();
  self->ensurePathExists();
  self->ensurePathExists();
  self->ensurePathExists();
  SUCCEED();
}  

TEST_F(FileSeriesFixture,EnsurePathExistsWithInc)
{
  self->ensurePathExists();
  self->inc();
  self->ensurePathExists();
  self->inc();
  self->ensurePathExists();
  SUCCEED();
}  
