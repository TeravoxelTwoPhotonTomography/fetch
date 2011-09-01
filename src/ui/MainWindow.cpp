#include "MainWindow.h"
#include <assert.h>
#include "common.h"
#include "devices/Microscope.h"
#include "VideoAcquisitionDockWidget.h"
#include "Figure.h"
#include "Player.h"
#include "StackAcquisitionDockWidget.h"
#include "MicroscopeStateDockWidget.h"
#include "VibratomeDockWidget.h"
#include "StageController.h"
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/tokenizer.h>

fetch::ui::MainWindow::MainWindow(device::Microscope *dc)
  :_dc(dc)
  ,quitAct(0)
  ,openAct(0)
  ,saveToAct(0)
  ,fullscreenAct(0)
  ,fullscreenStateOff(0)
  ,fullscreenStateOn(0)
  ,_videoAcquisitionDockWidget(0)
  ,_microscopesStateDockWidget(0)
  ,_vibratomeDockWidget(0)
  ,_display(0)
  ,_player(0)
  ,_scope_state_controller(&dc->__self_agent)
  ,_stageController(NULL)
  ,_resonant_turn_controller(NULL)
  ,_vlines_controller(NULL)
  ,_lsm_vert_range_controller(NULL)
  ,_pockels_controller(NULL)
{  

  _resonant_turn_controller = new ResonantTurnController(dc,"&Turn (px)");
  _vlines_controller        = new LinesController(dc,"Y &Lines (px)");
  _lsm_vert_range_controller= new LSMVerticalRangeController(dc->LSM(),"&Y Range (Vpp)");
  _pockels_controller       = new PockelsController(dc->pockels(),"&Pockels (mV)");

  _zpiezo_max_control       = new ZPiezoMaxController(dc->zpiezo(), "Z Ma&x (um)");
  _zpiezo_min_control       = new ZPiezoMinController(dc->zpiezo(), "Z Mi&n (um)");
  _zpiezo_step_control      = new ZPiezoStepController(dc->zpiezo(),"Z &Step (um)");

  _stageController          = new PlanarStageController(dc->stage());
  _vibratome_amp_controller = new VibratomeAmplitudeController(dc->vibratome(),"Amplitude (0-255)");
  _vibratome_feed_distance_controller = new VibratomeFeedDisController(dc->vibratome(),"Feed distance (mm)");
  _vibratome_feed_velocity_controller = new VibratomeFeedVelController(dc->vibratome(),"Feed velocity (mm/s)");

  createActions();
  createStateMachines();
  createMenus();
  createDockWidgets();
  createViews();
  load_settings_();
  
  connect(&_poller,SIGNAL(timeout()),&_scope_state_controller,SLOT(poll()));
  _poller.start(50 /*ms*/);  
}    

#define SAFE_DELETE(expr) if(expr) delete (expr)

fetch::ui::MainWindow::~MainWindow()
{
  SAFE_DELETE(_player);

  SAFE_DELETE(_resonant_turn_controller);
  SAFE_DELETE(_vlines_controller);
  SAFE_DELETE(_lsm_vert_range_controller);
  SAFE_DELETE(_pockels_controller);

  SAFE_DELETE(_zpiezo_max_control);
  SAFE_DELETE(_zpiezo_min_control);
  SAFE_DELETE(_zpiezo_step_control);
  
}

void fetch::ui::MainWindow::createActions()
{
  {
    fullscreenAct = new QAction(QIcon(":/icons/fullscreen"),"&Full Screen",this);
    fullscreenAct->setShortcut(Qt::CTRL + Qt::Key_F);
    fullscreenAct->setStatusTip("Display over full screen");
  }

  {
    quitAct = new QAction(QIcon(":/icons/quit"),"&Quit",this);
    QList<QKeySequence> quitShortcuts;
    quitShortcuts 
      << QKeySequence::Close
      << QKeySequence::Quit
      << (Qt::CTRL+Qt::Key_Q);
    quitAct->setShortcuts(quitShortcuts);
    quitAct->setStatusTip("Shutdown the microscope and exit.");
    connect(quitAct,SIGNAL(triggered()),this,SLOT(close()));
  }

  {
    openAct = new QAction(QIcon(":/icons/open"),"&Open",this);
    openAct->setShortcut(QKeySequence::Open);
    openAct->setStatusTip("Open a configuration file.");
    connect(openAct,SIGNAL(triggered()),this,SLOT(openMicroscopeConfig()));
    // TODO: connect to something
  }

  {
    saveToAct = new QAction(QIcon(":/icons/saveto"),"&Save To",this);
    saveToAct->setShortcut(QKeySequence::Save);
    saveToAct->setStatusTip("Save the configuration to a file."); 
    connect(saveToAct,SIGNAL(triggered()),this,SLOT(saveMicroscopeConfig()));
    // TODO:  connect to something
  }
}

void fetch::ui::MainWindow::createMenus()
{
  assert(fullscreenAct);
  assert(quitAct);
  assert(openAct);
  assert(saveToAct);
  
  fileMenu = menuBar()->addMenu("&File");
  fileMenu->addAction(openAct);
  fileMenu->addAction(saveToAct);
  fileMenu->addAction(quitAct);                                                                                                                                                                                                                                           

  viewMenu = menuBar()->addMenu("&View");
  viewMenu->addAction(fullscreenAct);
}

void fetch::ui::MainWindow::createStateMachines()
{
  assert(fullscreenAct);

  { //Fullscreen
    fullscreenStateOn  = new QState();
    fullscreenStateOff = new QState();
    connect(fullscreenStateOn, SIGNAL(entered()),this,SLOT(showFullScreen()));
    connect(fullscreenStateOff,SIGNAL(entered()),this,SLOT(showNormal()));  
    fullscreenStateOn->addTransition( fullscreenAct,SIGNAL(triggered()),fullscreenStateOff);
    fullscreenStateOff->addTransition(fullscreenAct,SIGNAL(triggered()),fullscreenStateOn);
  
    fullscreenStateMachine.addState(fullscreenStateOff);
    fullscreenStateMachine.addState(fullscreenStateOn);
    fullscreenStateMachine.setInitialState(fullscreenStateOff);
    fullscreenStateMachine.start();
  }
}

void fetch::ui::MainWindow::createDockWidgets()
{
  _videoAcquisitionDockWidget = new VideoAcquisitionDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_videoAcquisitionDockWidget);
  viewMenu->addAction(_videoAcquisitionDockWidget->toggleViewAction());
  _videoAcquisitionDockWidget->setObjectName("videoAcquisitionDockWidget");

  _stackAcquisitionDockWidget = new StackAcquisitionDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_stackAcquisitionDockWidget);
  viewMenu->addAction(_stackAcquisitionDockWidget->toggleViewAction());
  _stackAcquisitionDockWidget->setObjectName("_stackAcquisitionDockWidgetv");

  _microscopesStateDockWidget = new MicroscopeStateDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_microscopesStateDockWidget);
  viewMenu->addAction(_microscopesStateDockWidget->toggleViewAction());
  _microscopesStateDockWidget->setObjectName("_microscopesStateDockWidget");

  _vibratomeDockWidget = new VibratomeDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_vibratomeDockWidget);
  viewMenu->addAction(_vibratomeDockWidget->toggleViewAction());
  _vibratomeDockWidget->setObjectName("_vibratomeStateDockWidget");
}

void fetch::ui::MainWindow::createViews()
{
  _display = new Figure(_stageController);
  _player  = new AsynqPlayer(_dc->getVideoChannel(),_display);
  setCentralWidget(_display);
  connect(_videoAcquisitionDockWidget,SIGNAL(onRun()),
          _display,                   SLOT(fitNext()));
  connect(_stackAcquisitionDockWidget,SIGNAL(onRun()),
          _display,                   SLOT(fitNext()));
  _player->start();
}

void fetch::ui::MainWindow::closeEvent( QCloseEvent *event )
{
  _player->stop();
  _poller.stop();
  save_settings_();
  QMainWindow::closeEvent(event);  
}

void fetch::ui::MainWindow::load_settings_()
{
  QSettings settings;
  bool ok;
  ok  = restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
  ok &= restoreState(settings.value("MainWindow/state").toByteArray(),version__);
  ok &= restoreDockWidget(_videoAcquisitionDockWidget);
  ok &= restoreDockWidget(_stackAcquisitionDockWidget);
  ok &= restoreDockWidget(_microscopesStateDockWidget);
  ok &= restoreDockWidget(_vibratomeDockWidget);
}

void fetch::ui::MainWindow::save_settings_()
{
  QSettings settings;
  settings.setValue("MainWindow/geometry",saveGeometry());
  settings.setValue("MainWindow/state",saveState(version__));
}

#define CHKJMP(expr,lbl) if(!(expr)) goto lbl;
class MainWindowProtobufErrorCollector:public google::protobuf::io::ErrorCollector
{ public:
 
  virtual void AddError(int line, int col, const std::string & message)    {warning("Protobuf: Error   - at line %d(%d):"ENDL"\t%s"ENDL,line,col,message.c_str());}
  virtual void AddWarning(int line, int col, const std::string & message)  {warning("Protobuf: Warning - at line %d(%d):"ENDL"\t%s"ENDL,line,col,message.c_str());}
};

void fetch::ui::MainWindow::openMicroscopeConfig()
{
  const QString d("MainWindow/LastConfigDirectory");
  QSettings settings;
  QString filename = QFileDialog::getOpenFileName(this,
    tr("Load configuration"),
    settings.value(d,QDir::currentPath()).toString(),
    tr("Microscope Config (*.microscope);;Text Files (*.txt);;Any (*.*)"));
  if(filename.isEmpty())
    return;

  QFile cfgfile(filename);
  CHKJMP(cfgfile.open(QIODevice::ReadOnly), ErrorFileAccess);
  CHKJMP(cfgfile.isReadable()             , ErrorFileAccess);

  // store last good location back to settings
  { QFileInfo info(filename);
    settings.setValue(d,info.absoluteFilePath());
  }

  //load and parse
  { google::protobuf::TextFormat::Parser parser;
    MainWindowProtobufErrorCollector e;
    fetch::cfg::device::Microscope cfg;
    parser.RecordErrorsTo(&e);
  
    CHKJMP(parser.ParseFromString(cfgfile.readAll().constData(),&cfg),ParseError);

    // commit
    _dc->set_config(cfg);  
  }  

  return;
ErrorFileAccess:
  warning("(%d:%s) MainWindow - could not open file for reading."ENDL,__FILE__,__LINE__);
  return;
ParseError:
  warning("(%d:%s) MainWindow - could not parse config file."ENDL,__FILE__,__LINE__);
  return;
}

void fetch::ui::MainWindow::saveMicroscopeConfig()
{
  const QString d("MainWindow/LastConfigDirectory");
  QSettings settings;
  QString filename = QFileDialog::getSaveFileName(this,
    tr("Save configuration"),
    settings.value(d,QDir::currentPath()).toString(),
    tr("Microscope Config (*.microscope);;Text Files (*.txt);;Any (*.*)"));
  if(filename.isEmpty())
    return;


  {    
    QFile file(filename);
    CHKJMP(file.open(QIODevice::WriteOnly|QIODevice::Truncate),ErrorFileAccess);
    QTextStream fout(&file);

    std::string s;
    fetch::cfg::device::Microscope c = _dc->get_config();
    google::protobuf::TextFormat::PrintToString(c,&s);
    fout << s.c_str();
  //get_config().SerializePartialToOstream(&fout);
  }

  // store last good location back to settings
  { QFileInfo info(filename);
    settings.setValue(d,info.absoluteFilePath());  
    settings.setValue("Microscope/Config/DefaultFilename",info.absoluteFilePath());
  }

  // store as default config to be used next time
  

  return;
ErrorFileAccess:
  warning("(%d:%s) MainWindow - could not open file for writing."ENDL,__FILE__,__LINE__);
  return;
}
