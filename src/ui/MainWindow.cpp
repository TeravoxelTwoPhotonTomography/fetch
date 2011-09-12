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
#include <QtGui>

const char fetch::ui::MainWindow::defaultConfigPathKey[] = "Microscope/Config/DefaultFilename";

//////////////////////////////////////////////////////////////////////////////
// CONFIGFILENAMEDISPLAY
//////////////////////////////////////////////////////////////////////////////

fetch::ui::ConfigFileNameDisplay::
  ConfigFileNameDisplay(const QString& filename, QWidget *parent)
  : QLabel(parent)
{             
  setOpenExternalLinks(true);
  setTextFormat(Qt::RichText);
  setTextInteractionFlags(Qt::NoTextInteraction
      |Qt::TextBrowserInteraction
      |Qt::LinksAccessibleByKeyboard
      |Qt::LinksAccessibleByMouse);
  update(filename);
}

void 
  fetch::ui::ConfigFileNameDisplay::
  update(const QString& filename)
{ QString link = QString("Config: <a href=\"file:///%1\">%1</a>").arg(filename);
  setText(link);
}

//////////////////////////////////////////////////////////////////////////////
// FILESERIESNAMEFILENAMEDISPLAY
//////////////////////////////////////////////////////////////////////////////

fetch::ui::FileSeriesNameDisplay::
  FileSeriesNameDisplay(const QString& filename, QWidget *parent)
  : QLabel(parent)
{             
  setOpenExternalLinks(true);
  setTextFormat(Qt::RichText);
  setTextInteractionFlags(Qt::NoTextInteraction
      |Qt::TextBrowserInteraction
      |Qt::LinksAccessibleByKeyboard
      |Qt::LinksAccessibleByMouse);
  update(filename);

  connect(listener(),SIGNAL(sig_update(const QString&)),
                this,SLOT(update(const QString&)),
    Qt::QueuedConnection);
}

void 
  fetch::ui::FileSeriesNameDisplay::
  update(const QString& filename)
{ QString link = QString("FileSeries: <a href=\"file:///%1\">%1</a>").arg(filename);
  setText(link);
}

//////////////////////////////////////////////////////////////////////////////
// MAINWINDOW
//////////////////////////////////////////////////////////////////////////////

fetch::ui::MainWindow::MainWindow(device::Microscope *dc)
  :_dc(dc)
  ,quitAct(0)
  ,openAct(0)
  ,saveAct(0)
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

  _resonant_turn_controller = new ResonantTurnController(dc,"&Turn (px)",this);
  _vlines_controller        = new LinesController(dc,"Y &Lines (px)",this);
  _lsm_vert_range_controller= new LSMVerticalRangeController(dc->LSM(),"&Y Range (Vpp)",this);
  _pockels_controller       = new PockelsController(dc->pockels(),"&Pockels (mV)",this);

  _zpiezo_max_control       = new ZPiezoMaxController(dc->zpiezo(), "Z Ma&x (um)",this);
  _zpiezo_min_control       = new ZPiezoMinController(dc->zpiezo(), "Z Mi&n (um)",this);
  _zpiezo_step_control      = new ZPiezoStepController(dc->zpiezo(),"Z &Step (um)",this);

  _stageController          = new PlanarStageController(dc->stage());
  _vibratome_amp_controller = new VibratomeAmplitudeController(dc->vibratome(),"Amplitude (0-255)",this);
  _vibratome_feed_distance_controller = new VibratomeFeedDisController(dc->vibratome(),"Feed distance (mm)",this);
  _vibratome_feed_velocity_controller = new VibratomeFeedVelController(dc->vibratome(),"Feed velocity (mm/s)",this);
  _vibratome_feed_axis_controller     = new VibratomeFeedAxisController(dc->vibratome(),"Feed Axis",this);

  _config_watcher = new QFileSystemWatcher(this);
  connect(_config_watcher, SIGNAL(fileChanged(const QString&)),
                     this,   SLOT(openMicroscopeConfigDelayed(const QString&)));

  createActions();
  createStateMachines();
  createMenus();
  createDockWidgets();
  createViews();
  load_settings_();
   
  connect(&_poller,SIGNAL(timeout()),&_scope_state_controller,SLOT(poll()));
  connect(&_scope_state_controller,SIGNAL(onArm()),this,SLOT(startVideo()));
  connect(&_scope_state_controller,SIGNAL(onDisarm()),this,SLOT(stopVideo()));
  _poller.start(50 /*ms*/);

  { QStatusBar *bar = statusBar();
    { QString f = _config_watcher->files().at(0);
      ConfigFileNameDisplay *w = new ConfigFileNameDisplay(f);    
      bar->addPermanentWidget(w);
      connect(this,SIGNAL(configFileChanged(const QString&)),w,SLOT(update(const QString&)));
    }
    { QString f(_dc->file_series.getPath().c_str());
      FileSeriesNameDisplay *w = new FileSeriesNameDisplay(f);
      _dc->file_series.addListener(w->listener());
      bar->addWidget(w);      
    }

    bar->setSizeGripEnabled(true);
  }
}    

#define SAFE_DELETE(expr) if(expr) delete (expr)
#define SAFE_DELETE_THREAD(expr) if(expr) { (expr)->wait(); delete (expr); }

fetch::ui::MainWindow::~MainWindow()
{
  SAFE_DELETE_THREAD(_player);

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
    connect(openAct,SIGNAL(triggered()),this,SLOT(openMicroscopeConfigViaFileDialog()));
  }

  {
    saveAct = new QAction(QIcon(":/icons/save"),"&Save",this);
    saveAct->setShortcut(QKeySequence::Save);
    saveAct->setStatusTip("Save the configuration to the last loaded location."); 
    connect(saveAct,SIGNAL(triggered()),this,SLOT(saveMicroscopeConfigToLastGoodLocation()));
  }

  {
    saveToAct = new QAction(QIcon(":/icons/saveas"),"Save &As",this);
    saveToAct->setShortcut(QKeySequence::SaveAs);
    saveToAct->setStatusTip("Save the configuration to a file."); 
    connect(saveToAct,SIGNAL(triggered()),this,SLOT(saveMicroscopeConfigViaFileDialog()));
  }
}

void fetch::ui::MainWindow::createMenus()
{
  assert(fullscreenAct);
  assert(quitAct);
  assert(openAct);
  assert(saveAct);
  assert(saveToAct);
  
  fileMenu = menuBar()->addMenu("&File");
  fileMenu->addAction(openAct);
  fileMenu->addAction(saveAct);
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

  _vibratomeGeometryDockWidget = new VibratomeGeometryDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_vibratomeGeometryDockWidget);
  viewMenu->addAction(_vibratomeGeometryDockWidget->toggleViewAction());
  _vibratomeGeometryDockWidget->setObjectName("_vibratomeGeometryDockWidget");
}

void fetch::ui::MainWindow::createViews()
{
  _display = new Figure(_stageController);
  
  setCentralWidget(_display);
  connect(_videoAcquisitionDockWidget,SIGNAL(onRun()),
          _display,                   SLOT(fitNext()));
  connect(_stackAcquisitionDockWidget,SIGNAL(onRun()),
          _display,                   SLOT(fitNext()));
  //_player->start();
}

void fetch::ui::MainWindow::closeEvent( QCloseEvent *event )
{ 
  QMainWindow::closeEvent(event);   // must come before stopVideo
  stopVideo();
  _dc->stage()->delListener(_stageController->listener());
  _dc->stage()->delListener(_stageController->tiling()->listener());
  _poller.stop();
  save_settings_();  
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

  { QString filename = settings.value(defaultConfigPathKey,":/config/microscope").toString();
    QStringList files = _config_watcher->files();
    if(!files.isEmpty())
      _config_watcher->removePaths(files);  
    _config_watcher->addPath(filename);
  }
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

void fetch::ui::MainWindow::openMicroscopeConfigViaFileDialog()
{ QSettings settings;
  QString filename = QFileDialog::getOpenFileName(this,
    tr("Load configuration"),
    settings.value(defaultConfigPathKey,QDir::currentPath()).toString(),
    tr("Microscope Config (*.microscope);;Text Files (*.txt);;Any (*.*)"));
  if(filename.isEmpty())
    return;

  openMicroscopeConfig(filename);
}

void 
  fetch::ui::MainWindow::
  openMicroscopeConfigDelayed(const QString& filename)
{ 
  HERE;
  qDebug() << _config_watcher->files();

  // cleanup old signals
  _config_delayed_load_timer.disconnect();
  _config_delayed_load_mapper.disconnect();
  _config_delayed_load_mapper.removeMappings(&_config_delayed_load_timer);
  // init the new ones
  _config_delayed_load_mapper.setMapping(&_config_delayed_load_timer,filename);         // Use a signal mapper to 
  connect(&_config_delayed_load_timer , SIGNAL(timeout()),                              // send the string after 
          &_config_delayed_load_mapper, SLOT(map()));                                   // a specified delay.
  connect(&_config_delayed_load_mapper, SIGNAL(mapped(const QString&)),                 
                                  this, SLOT(openMicroscopeConfig(const QString&)));    

  _config_delayed_load_timer.setSingleShot(true);
  _config_delayed_load_timer.setInterval(100/*ms*/);
  _config_delayed_load_timer.start();
}

void 
  fetch::ui::MainWindow::
  openMicroscopeConfig(const QString& filename)
{ QFile cfgfile(filename);

  CHKJMP(cfgfile.open(QIODevice::ReadOnly), ErrorFileAccess);
  CHKJMP(cfgfile.isReadable()             , ErrorFileAccess);

  //load and parse
  { google::protobuf::TextFormat::Parser parser;
    MainWindowProtobufErrorCollector e;
    fetch::cfg::device::Microscope cfg;
    parser.RecordErrorsTo(&e);
  
    CHKJMP(parser.ParseFromString(cfgfile.readAll().constData(),&cfg),ParseError);

    // commit
    _dc->set_config(cfg);
    emit configUpdated();
  }  

  update_active_config_location_(filename);

  return;
ErrorFileAccess:
  warning("%s(%d) MainWindow"ENDL
          "\tCould not open file for reading."ENDL
          "\tFile name: %s"ENDL,
          __FILE__,__LINE__,filename.toLocal8Bit().data());
  return;
ParseError:
  warning("%s(%d) MainWindow - could not parse config file."ENDL,__FILE__,__LINE__);
  update_active_config_location_(filename); // keep watching the file
  return;
}

/*
  void saveMicroscopeConfigViaFileDialog();
  void saveMicroscopeConfigToLastGoodLocation();
  void saveMicroscopeConfig(const QString& filename);
*/

void
  fetch::ui::MainWindow::
  saveMicroscopeConfigViaFileDialog()
{
  QSettings settings;
  QString filename = QFileDialog::getSaveFileName(this,
    tr("Save configuration"),
    settings.value(defaultConfigPathKey,QDir::currentPath()).toString(),
    tr("Microscope Config (*.microscope);;Text Files (*.txt);;Any (*.*)"));
  if(filename.isEmpty())
    return;

  saveMicroscopeConfig(filename);
}

void 
  fetch::ui::MainWindow::
  saveMicroscopeConfigToLastGoodLocation()
{ QSettings s;
  QVariant v = s.value(defaultConfigPathKey);
  if(v.isValid())
    saveMicroscopeConfig(v.toString());
  else
    warning("%s(%d): Attempted save to last config location, but there was no last config location."ENDL,__FILE__,__LINE__);
}

void
  fetch::ui::MainWindow::
  saveMicroscopeConfig(const QString& filename)
{    
  QFile file(filename);
  CHKJMP(file.open(QIODevice::WriteOnly|QIODevice::Truncate),ErrorFileAccess);
    
  { QTextStream fout(&file);  
    std::string s;
    google::protobuf::TextFormat::PrintToString(_dc->get_config(),&s);
    fout << s.c_str();
  }

  update_active_config_location_(filename);
  
  return;
ErrorFileAccess:
  warning("(%d:%s) MainWindow - could not open file for writing."ENDL,__FILE__,__LINE__);
  return;
}

void 
  fetch::ui::MainWindow::
  update_active_config_location_(const QString& filename)
{
  // store location back to settings - use as default next time GUI is opened
  { QFileInfo info(filename);
    QSettings settings;
    settings.setValue(defaultConfigPathKey,info.absoluteFilePath());  
    //settings.setValue("Microscope/Config/DefaultFilename",info.absoluteFilePath());
  }

  // Update watcher
  QStringList files = _config_watcher->files();
  if(!files.isEmpty())
    _config_watcher->removePaths(files);  
  _config_watcher->addPath(filename);
  
  emit configFileChanged(filename);
}

void
  fetch::ui::MainWindow::
  startVideo()
{ if(_player)
    error("%s(%d):"ENDL"\t_player should be NULL"ENDL,__FILE__,__LINE__);

  _player  = new AsynqPlayer(_dc->getVideoChannel(),_display);
  _player->start();
}

void
  fetch::ui::MainWindow::
  stopVideo()
{ if(!_player) return;
  _player->stop();
  _player->disconnect();
  _player->wait(); // hrmmmmmmmmmmmmmmmm...looks like trouble
  /*
  I think the problem is:
  1. Need to wait for stop before deleting old player
  2. wait for player stop blocks main thread
  3. Figure widget must run in main thread and waits for the player to emit the next image
  4. deadlock: player waiting for figure to accept, main thread is stuck here so figure can't accept

  Solution:
  a. don't delete player.  just swap channels when necessary.  must allow for a null channel.
  b. will disconnecting the signal prevent the wait?  If so then, _player->stop(),_player->disconnect(),_player->wait() should
     be ok.
      --- maybe this (b) worked.
  c. try using deleteLater()

  - May still get a rare deadlock? although the last time I got a deadlock here using a DirectConnection helped
  */
  delete _player;
  _player = NULL;
}
