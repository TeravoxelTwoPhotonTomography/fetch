/* =====
 * Fetch
 * =====
 *
 */

// Glew must be included before OpenGL
#ifdef _WIN32
#include <GL/glew.h>
#define INIT_EXTENSIONS \
  assert(glewInit()==GLEW_OK)
#else
#define INIT_EXTENSIONS
#endif

#include <QtGui>
#include <QtOpenGL>
#include <assert.h>
#include "util/util-gl.h"
#include "util/util-qt.h"

#include "common.h"
#include "devices/Microscope.h"
#include "tasks/microscope-interaction.h"

#include "ui/MainWindow.h"
#include <stdio.h>
/*
 * Global state
 */

fetch::device::Microscope *gp_microscope;
fetch::task::microscope::Interaction g_microscope_default_task;

/*
 * Shutdown callbacks
 */

unsigned int QtShutdownCallback(void)
{ QCoreApplication::exit(0); // Pass success (0), since I don't know what else to do
  return 0;
}

unsigned int KillMicroscopeCallback(void)
{
  if(gp_microscope) { delete gp_microscope; gp_microscope=NULL;}
  return 0;
}

void Init(void)
{
  gp_microscope = NULL;

  //Shutdown
  Register_New_Shutdown_Callback(QtShutdownCallback);
  Register_New_Shutdown_Callback(KillMicroscopeCallback);

  //Logging
  Reporting_Setup_Log_To_VSDebugger_Console();
  Reporting_Setup_Log_To_Filename("lastrun.log");
  Reporting_Setup_Log_To_Qt();

  //Microscope devices
  gp_microscope = new fetch::device::Microscope;


  // Connect video display
  // TODO
}

int main(int argc, char *argv[])
{ QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);

  QApplication app(argc,argv);
  fetch::ui::MainWindow mainwindow;

  QGraphicsScene scene;
  QGraphicsView  view(&scene);

  // This will eventually be the viewport for the view.  Make it here so we
  // can go ahead and make the GL context current.
  QGLWidget *viewport = new QGLWidget;

  view.setBackgroundBrush(QBrush(QColor("black")));
  
  // Make sure OpenGL gets set up right
  viewport->makeCurrent();
  INIT_EXTENSIONS;
  printOpenGLInfo();
  checkGLError();

  Init();

  mainwindow.setCentralWidget(&view);
  mainwindow.show();

  unsigned int eflag = app.exec();
  eflag |= Shutdown_Soft();
  debug("Press <Enter> to exit.\r\n");
  getchar();
  return eflag;
}
