/* =====
 * Fetch
 * =====
 *
 */

// Glew must be included before OpenGL
#ifdef _WIN32
//#include <GL/glew.h>
#define INIT_EXTENSIONS \
  assert(glewInit()==GLEW_OK)
#else
#define INIT_EXTENSIONS
#endif

#include <QtWidgets>
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
  QSettings settings;
  gp_microscope = NULL;
  fetch::cfg::device::Microscope* g_config=new fetch::cfg::device::Microscope();  // Don't free, should have application lifetime

  //Shutdown
  Register_New_Shutdown_Callback(KillMicroscopeCallback);
  Register_New_Shutdown_Callback(QtShutdownCallback);

  //Set Session Directory
  {
      bool ok=0;
      char buf[1024]={0};
      int session_id=settings.value("session_id").toInt(&ok);
      if(!ok)
        session_id=0;
      session_id=(session_id+1)&0xffff; // limit to 65k sessions
      settings.setValue("session_id",session_id);
      snprintf(buf,sizeof(buf),"Session-%05d",session_id);
      CreateDirectoryA(buf,0);
      SetCurrentDirectoryA(buf);
  }

  //Logging
  Reporting_Setup_Log_To_VSDebugger_Console();
  Reporting_Setup_Log_To_Filename("lastrun.log");
  Reporting_Setup_Log_To_Qt();
 
  google::protobuf::TextFormat::Parser parser;
  MyErrorCollector e;
  parser.RecordErrorsTo(&e);
  
  { QFile cfgfile(settings.value(fetch::ui::MainWindow::defaultConfigPathKey,":/config/microscope").toString());
    
    if(  cfgfile.open(QIODevice::ReadOnly) && cfgfile.isReadable() )
    { QByteArray contents=cfgfile.readAll();      
      qDebug() << contents;
      if(parser.ParseFromString(contents.constData(),g_config))
      { QByteArray buf=cfgfile.fileName().toLocal8Bit();
        debug("Config file loaded from %s\n",buf.data());
        goto Success;
      }      
    }
  }
  
  {
    //try again from default
    QFile cfgfile(":/config/microscope");
    Guarded_Assert(cfgfile.open(QIODevice::ReadOnly));
    Guarded_Assert(cfgfile.isReadable());
    Guarded_Assert(parser.ParseFromString(cfgfile.readAll().constData(),g_config));
    { QByteArray buf=cfgfile.fileName().toLocal8Bit();
      debug("Config file loaded from %s\n",buf.data());
    }
    settings.setValue(fetch::ui::MainWindow::defaultConfigPathKey,":/config/microscope");
  }
  //cfgfile.setTextModeEnabled(true);

Success:
  gp_microscope = new fetch::device::Microscope(g_config);
}

int main(int argc, char *argv[])
{ //[deprecated] -- QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
  QCoreApplication::setOrganizationName("Howard Hughes Medical Institute");
  QCoreApplication::setOrganizationDomain("janelia.hhmi.org");
  QCoreApplication::setApplicationName("Fetch");
  qDebug()<<QCoreApplication::libraryPaths();

  QApplication app(argc,argv);  
  Init();
  fetch::ui::MainWindow mainwindow(gp_microscope);  
  gp_microscope->onUpdate(); // force update so gui gets notified - have to do this mostly for stage listeners ...
  mainwindow.show();

  unsigned int eflag = app.exec();
  eflag |= Shutdown_Soft();
  //debug("Press <Enter> to exit.\r\n");
  //getchar();
  return eflag;
}
