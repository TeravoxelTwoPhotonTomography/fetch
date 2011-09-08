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
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/tokenizer.h>
#include <string>
/*
 * Global state
 */

fetch::device::Microscope*           gp_microscope;
fetch::cfg::device::Microscope       g_config;
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

class MyErrorCollector:public google::protobuf::io::ErrorCollector
{ public:
 
  virtual void AddError(int line, int col, const std::string & message)    {warning("Protobuf: Error   - at line %d(%d):"ENDL"\t%s"ENDL,line,col,message.c_str());}
  virtual void AddWarning(int line, int col, const std::string & message)  {warning("Protobuf: Warning - at line %d(%d):"ENDL"\t%s"ENDL,line,col,message.c_str());}
};

void Init(void)
{
  gp_microscope = NULL;

  //Shutdown
  Register_New_Shutdown_Callback(KillMicroscopeCallback);
  Register_New_Shutdown_Callback(QtShutdownCallback);

  //Logging
  Reporting_Setup_Log_To_VSDebugger_Console();
  Reporting_Setup_Log_To_Filename("lastrun.log");
  Reporting_Setup_Log_To_Qt();
 
  google::protobuf::TextFormat::Parser parser;
  MyErrorCollector e;
  parser.RecordErrorsTo(&e);
  QSettings settings;
  { QFile cfgfile(settings.value(fetch::ui::MainWindow::defaultConfigPathKey,":/config/microscope").toString());
    
    if(  cfgfile.open(QIODevice::ReadOnly) 
      && cfgfile.isReadable() 
      && parser.ParseFromString(cfgfile.readAll().constData(),&g_config))
    {
      qDebug() << "Config file loaded from " << cfgfile.fileName();
      goto Success;
    }      
  }
  
  {
    //try again from default
    QFile cfgfile(":/config/microscope");
    Guarded_Assert(cfgfile.open(QIODevice::ReadOnly));
    Guarded_Assert(cfgfile.isReadable());
    Guarded_Assert(parser.ParseFromString(cfgfile.readAll().constData(),&g_config));
    qDebug() << "Config file loaded from " << cfgfile.fileName();
    settings.setValue(fetch::ui::MainWindow::defaultConfigPathKey,":/config/microscope");
  }
  //cfgfile.setTextModeEnabled(true);

Success:
  gp_microscope = new fetch::device::Microscope(&g_config);
}

int main(int argc, char *argv[])
{ QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
  QCoreApplication::setOrganizationName("Howard Hughes Medical Institute");
  QCoreApplication::setOrganizationDomain("janelia.hhmi.org");
  QCoreApplication::setApplicationName("Fetch");

  QApplication app(argc,argv);
  Init();
  fetch::ui::MainWindow mainwindow(gp_microscope);
  gp_microscope->set_config_nowait(g_config); // reload so gui gets notified - have to do this mostly for stage listeners ...
  mainwindow.show();

  unsigned int eflag = app.exec();
  eflag |= Shutdown_Soft();
  //debug("Press <Enter> to exit.\r\n");
  //getchar();
  return eflag;
}
