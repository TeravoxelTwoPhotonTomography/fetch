#include <gtest/gtest.h>
#include <NIDAQmx.h>
#include <niscope.h>
#include "digitizer.pb.h"
#include <GL/glew.h>
#include <QtGui>
#include <QtOpenGL>

namespace mylib {
#include "array.h"
}

TEST(Integration,NIDAQmx)
{ TaskHandle task;
  int32 sts;
  EXPECT_EQ(0, sts = DAQmxCreateTask("test",&task));
  EXPECT_EQ(0, sts = DAQmxClearTask(task));
}

TEST(Integration,Protobuf)
{ fetch::cfg::device::Digitizer d;
  ASSERT_TRUE(d.IsInitialized());
}

TEST(Integration,NIScope)
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

TEST(Integration,Mylib)
{ mylib::Dimn_Type dims[] = {512,512,512}; //Allocate 125 Mvox * 4B/vox = 500 GB
  mylib::Array *array = mylib::Make_Array(mylib::PLAIN_KIND,mylib::FLOAT32,3,dims);
  EXPECT_FALSE(array==NULL);
  Kill_Array(array);
}

class GuiIntegration : public ::testing::Test
{
  protected:
    virtual void SetUp()
    { int argc=0;
      char *argv[] = { "commandline", NULL};
      app = new QApplication(argc,argv);
    }
    virtual void TearDown()
    { if(app) delete app;
    }

    QApplication *app;
};

TEST_F(GuiIntegration,Qt)
{ EXPECT_FALSE(app==NULL);
}

#include <stdio.h>
TEST_F(GuiIntegration,OpenGL)
{ QGLWidget *w = new QGLWidget;
  ASSERT_TRUE( w!=NULL );
  w->makeCurrent();
  const GLubyte *vendor = glGetString(GL_VENDOR);
  printf("GL_VENDOR: %s\n",vendor);
  EXPECT_TRUE(vendor!=NULL);
  EXPECT_STRNE("",(char*)vendor);
}

TEST_F(GuiIntegration,GLEW)
{ QGLWidget *w = new QGLWidget;
  ASSERT_TRUE( w!=NULL );
  w->makeCurrent();
  const GLubyte *version = glewGetString(GLEW_VERSION);
  printf("GLEW Version: %s\n",version);
  EXPECT_TRUE(version!=NULL);
  EXPECT_STRNE("",(char*)version);
}

