#include "MainWindow.h"
#include "MainWindow.h"
#include <assert.h>
#include "common.h"
#include "devices/Microscope.h"
#include "ColormapDockWidget.h"
#include "VideoAcquisitionDockWidget.h"
#include "Figure.h"
#include "Player.h"
#include "StackAcquisitionDockWidget.h"
#include "MicroscopeStateDockWidget.h"
#include "MicroscopeTcpServerDockWidget.h"
#include "SurfaceScanDockWidget.h"
#include "VibratomeDockWidget.h"
#include "StageDockWidget.h"
#include "AutoTileDockWidget.h"
#include "HistogramDockWidget.h"
#include "TimeSeriesDockWidget.h"
#include "AdaptiveTilingdockWidget.h"
#include "StageController.h"
#include "SurfaceFindDockWidget.h"
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/tokenizer.h>
#include <QtWidgets>

#define ENDL "\r\n"
#define TRY(e) if (!(e)) qFatal("%s(%d): "ENDL "\tExpression evaluated as false."ENDL "\t%s",__FILE__,__LINE__,#e)

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
{
  // Link to each folder in the path individually
  QString sep = QDir::separator(); // FIXME: for consistency, should get/update this from the file_series.path_sep value in the microscope config.
  QStringList names = filename.split(sep,QString::SkipEmptyParts);
  QString text = "FileSeries: ";
  QString path;
  foreach(QString n,names)
  {
    n+="/"; // Needs to be this slash for specifying the url (Qt5+)
    path += n;
    QString link = QString("<a href=\"file:///%1\">%2</a> ").arg(path,n);
    text += link;
  }
  setText(text);
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
  ,_cmapDockWidget(0)
  ,_videoAcquisitionDockWidget(0)
  ,_microscopesStateDockWidget(0)
  ,_microscopeTcpServer(0)
  ,_vibratomeDockWidget(0)
  ,_cutTaskDockWidget(0)
  ,_stageDockWidget(0)
  ,_surfaceScanDockWidget(0)
  ,_autoTileDockWidget(0)
  ,_timeSeriesDockWidget(0)
  ,_histogramDockWidget(0)
  ,_display(0)
  ,_player(0)
  ,_scope_state_controller(&dc->__self_agent)
  ,_stageController(NULL)
  ,_resonant_turn_controller(NULL)
  ,_vlines_controller(NULL)
  ,_lsm_vert_range_controller(NULL)
  ,_defered_update(0)
{

  _resonant_turn_controller = new ResonantTurnController(dc,"&Turn (px)",this);
  _vlines_controller        = new LinesController(dc,"Y &Lines (px)",this);
  _lsm_vert_range_controller= new LSMVerticalRangeController(dc->LSM(),"&Y Range (Vpp)",this);
  _pockels_controllers[0]    = new PockelsController(dc->pockels("Chameleon"),"&Chameleon Pockels (%)",this);
  _pockels_controllers[1]    = new PockelsController(dc->pockels("Fianium"),"&Fianium Pockels (%)",this);

  _zpiezo_max_control       = new ZPiezoMaxController(dc->zpiezo(), "Z Ma&x (um)",this);
  _zpiezo_min_control       = new ZPiezoMinController(dc->zpiezo(), "Z Mi&n (um)",this);
  _zpiezo_step_control      = new ZPiezoStepController(dc->zpiezo(),"Z &Step (um)",this);

  _stageController          = new PlanarStageController(dc->stage());
  _vibratome_amp_controller = new VibratomeAmplitudeController(dc->vibratome(),"Amplitude (0-255)",this);
  _vibratome_feed_distance_controller = new VibratomeFeedDisController(dc->vibratome(),"Feed distance (mm)",this);
  _vibratome_feed_velocity_controller = new VibratomeFeedVelController(dc->vibratome(),"Feed velocity (mm/s)",this);
  _vibratome_feed_axis_controller     = new VibratomeFeedAxisController(dc->vibratome(),"Feed Axis",this);
  _vibratome_feed_pos_x_controller    = new VibratomeFeedPosXController(dc->vibratome(),"Cut Pos X (mm)", this);
  _vibratome_feed_pos_y_controller    = new VibratomeFeedPosYController(dc->vibratome(),"Cut Pos Y (mm)", this);
  _vibratome_z_offset_controller      = new VibratomeZOffsetController(dc->vibratome(),"Z Offset (mm)", this);
  _vibratome_thick_controller         = new VibratomeThickController(dc->vibratome(),"Slice Thickness (um)", this);

  _stage_pos_x_control = new StagePosXController(dc->stage(),"Pos X (mm)",this);
  _stage_pos_y_control = new StagePosYController(dc->stage(),"Pos Y (mm)",this);
  _stage_pos_z_control = new StagePosZController(dc->stage(),"Pos Z (mm)",this);
  _stage_vel_x_control = new StageVelXController(dc->stage(),"Vel X (mm)",this);
  _stage_vel_y_control = new StageVelYController(dc->stage(),"Vel Y (mm)",this);
  _stage_vel_z_control = new StageVelZController(dc->stage(),"Vel Z (mm)",this);

  _fov_overlap_z_controller = new FOVOverlapZController(&dc->fov_,"Overlap Z (um)", this);

  _autotile_zoffum_control           = new AutoTileZOffController(dc,"Z Offset (um)",this);
  _autotile_zmaxmm_control           = new AutoTileZMaxController(dc,"Cut to Max Z (mm)",this);
  _autotile_timeoutms_control        = new AutoTileTimeoutMsController(dc,"Timeout (ms)",this);
  _autotile_chan_control             = new AutoTileChanController(dc,"Channel to threshold",this);
  _autotile_intensity_thresh_control = new AutoTileIntensityThresholdController(dc,"Intensity threshold",this);
  _autotile_area_thresh_control      = new AutoTileAreaThresholdController(dc,"Area threshold (0-1)",this);

  connect(_stageController,SIGNAL(moved()),          _stage_pos_x_control,SIGNAL(configUpdated()));
  connect(_stageController,SIGNAL(moved()),          _stage_pos_y_control,SIGNAL(configUpdated()));
  connect(_stageController,SIGNAL(moved()),          _stage_pos_z_control,SIGNAL(configUpdated()));
  connect(_stageController,SIGNAL(velocityChanged()),_stage_vel_x_control,SIGNAL(configUpdated()));
  connect(_stageController,SIGNAL(velocityChanged()),_stage_vel_y_control,SIGNAL(configUpdated()));
  connect(_stageController,SIGNAL(velocityChanged()),_stage_vel_z_control,SIGNAL(configUpdated()));

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
    { QString f("Default");
      if(!_config_watcher->files().isEmpty())
        f = _config_watcher->files().at(0);
      ConfigFileNameDisplay *w = new ConfigFileNameDisplay(f);
      bar->addPermanentWidget(w);
      connect(this,SIGNAL(configFileChanged(const QString&)),w,SLOT(update(const QString&)));
    }
    { std::string s=_dc->file_series.getPath();
      QString f(s.c_str());
      FileSeriesNameDisplay *w = new FileSeriesNameDisplay(f);
      _dc->file_series.addListener(w->listener());
      bar->addWidget(w);
    }

    bar->setSizeGripEnabled(true);
  }

  _defered_update = new internal::mainwindow_defered_update(this);
}

#define SAFE_DELETE(expr) if(expr) delete (expr)
#define SAFE_DELETE_THREAD(expr) if(expr) { (expr)->wait(); delete (expr); }

fetch::ui::MainWindow::~MainWindow()
{
  SAFE_DELETE_THREAD(_player);

  SAFE_DELETE(_resonant_turn_controller);
  SAFE_DELETE(_vlines_controller);
  SAFE_DELETE(_lsm_vert_range_controller);
  SAFE_DELETE(_pockels_controllers[0]);
  SAFE_DELETE(_pockels_controllers[1]);

  SAFE_DELETE(_zpiezo_max_control);
  SAFE_DELETE(_zpiezo_min_control);
  SAFE_DELETE(_zpiezo_step_control);

  SAFE_DELETE(_defered_update);

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

  QMenu *t = menuBar()->addMenu("&Tiling");
  t->addAction(tilingController()->loadDialogAction());
  t->addAction(tilingController()->saveDialogAction());
  t->addAction(tilingController()->autosaveAction());

  { QMenu *t = menuBar()->addMenu("&Actions");
    QAction *a = new QAction("&Reset Trip Detector",this);
    t->addAction(a);
    connect(a,SIGNAL(triggered()),this,SLOT(resetTripDetector()));

    a=new QAction("Reset &PMT Controller",this);
    t->addAction(a);
    connect(a,SIGNAL(triggered()),this,SLOT(resetPMTController()));
  }
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
  _cmapDockWidget = new ColormapDockWidget(this);
  addDockWidget(Qt::LeftDockWidgetArea,_cmapDockWidget);
  viewMenu->addAction(_cmapDockWidget->toggleViewAction());
  _cmapDockWidget->setObjectName("cmapDockWidget");

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

  _microscopeTcpServer=new MicroscopeTcpServerDockWidget(_dc,&_scope_state_controller,this);
  addDockWidget(Qt::LeftDockWidgetArea,_microscopeTcpServer);
  viewMenu->addAction(_microscopeTcpServer->toggleViewAction());
  _microscopeTcpServer->setObjectName("_microscopeTcpServer");


  _stageDockWidget = new StageDockWidget(_dc,this);             // has to come before the _vibratomeDockWidget
  addDockWidget(Qt::LeftDockWidgetArea,_stageDockWidget);
  viewMenu->addAction(_stageDockWidget->toggleViewAction());
  _stageDockWidget->setObjectName("_stageDockWidget");

  _vibratomeDockWidget = new VibratomeDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_vibratomeDockWidget);
  viewMenu->addAction(_vibratomeDockWidget->toggleViewAction());
  _vibratomeDockWidget->setObjectName("_vibratomeStateDockWidget");

  _surfaceScanDockWidget = new SurfaceScanDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_surfaceScanDockWidget);
  viewMenu->addAction(_surfaceScanDockWidget->toggleViewAction());
  _surfaceScanDockWidget->setObjectName("_surfaceScanDockWidget");

  {
    QDockWidget *w = new SurfaceFindDockWidget(_dc,this);
    addDockWidget(Qt::LeftDockWidgetArea,w);
    viewMenu->addAction(w->toggleViewAction());
    w->setObjectName("_surfaceFindDockWidget");
  }

  {
    QDockWidget *w = new AdaptiveTilingDockWidget(_dc,this);
    addDockWidget(Qt::LeftDockWidgetArea,w);
    viewMenu->addAction(w->toggleViewAction());
    w->setObjectName("_AdaptiveTiledAcquisition");
  }

  _timeSeriesDockWidget = new TimeSeriesDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_timeSeriesDockWidget);
  viewMenu->addAction(_timeSeriesDockWidget->toggleViewAction());
  _timeSeriesDockWidget->setObjectName("_timeSeriesDockWidget");

  _vibratomeGeometryDockWidget = new VibratomeGeometryDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_vibratomeGeometryDockWidget);
  viewMenu->addAction(_vibratomeGeometryDockWidget->toggleViewAction());
  _vibratomeGeometryDockWidget->setObjectName("_vibratomeGeometryDockWidget");

  _cutTaskDockWidget = _scope_state_controller.createTaskDockWidget("Cut Cycle",&_dc->cut_task);
  addDockWidget(Qt::LeftDockWidgetArea,_cutTaskDockWidget);
  viewMenu->addAction(_cutTaskDockWidget->toggleViewAction());
  _cutTaskDockWidget->setObjectName("_cutTaskDockWidget");

  _autoTileDockWidget = new AutoTileDockWidget(_dc,this);
  addDockWidget(Qt::LeftDockWidgetArea,_autoTileDockWidget);
  viewMenu->addAction(_autoTileDockWidget->toggleViewAction());
  _autoTileDockWidget->setObjectName("_autoTileDockWidget");

  { HistogramDockWidget *w = new HistogramDockWidget(this);
    addDockWidget(Qt::LeftDockWidgetArea,w);
    viewMenu->addAction(w->toggleViewAction());
    w->setObjectName("_histogramDockWidget");
    _histogramDockWidget=w;
  }

}

void fetch::ui::MainWindow::createViews()
{
  _display = new Figure(_stageController);

  setCentralWidget(_display);
  TRY(connect(_videoAcquisitionDockWidget,SIGNAL(onRun()),
              _display,                   SLOT(fitNext())));
  TRY(connect(_stackAcquisitionDockWidget,SIGNAL(onRun()),
              _display,                   SLOT(fitNext())));

  TRY(connect(_cmapDockWidget,SIGNAL(colormapSelectionChanged(const QString&)),
              _display,       SLOT(setColormap(const QString&))));
  TRY(connect(_cmapDockWidget,SIGNAL(gammaChanged(float)),
              _display,       SLOT(setGamma(float))));
  _display->setColormap(_cmapDockWidget->cmap());
  //_player->start();
}

int fetch::ui::MainWindow::maybeSave()
{
  int ret = QMessageBox::warning(this,
    "Fetch",
    "Save configuration data before closing?",
    QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
  switch(ret)
  {
  case QMessageBox::Save:
    saveToAct->trigger();
    tilingController()->saveDialogAction()->trigger();
    break;
  case QMessageBox::Discard:
    break;
  case QMessageBox::Cancel:
  default:
    return 0;
  }
  return 1;
}

void fetch::ui::MainWindow::closeEvent( QCloseEvent *event )
{
  if(!maybeSave())
  { event->ignore(); // cancel the close
    return;
  }

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
    if(_defered_update)
      _defered_update->update(cfg); // set_config_nowait won't work here.
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
  resetTripDetector()
{ _dc->trip_detect.reset();
}

void
  fetch::ui::MainWindow::
  resetPMTController()
{ _dc->pmt_.reset();
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

  { device::Microscope::Config cfg(_dc->get_config());
    QTextStream fout(&file);
    std::string s;
    google::protobuf::TextFormat::PrintToString(cfg,&s);
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
  TRY(connect(
      _player             ,SIGNAL(imageReady(mylib::Array*)),
      _histogramDockWidget,SLOT  (set(mylib::Array*)),
      Qt::BlockingQueuedConnection));
Error:
  _player->start();
}

void
  fetch::ui::MainWindow::
  stopVideo()
{ if(!_player) return;
  _dead_players.enqueue(_player);
  _player->disconnect();
  _display->disconnect();
  _histogramDockWidget->disconnect(_player,SIGNAL(imageReady(mylib::Array*)));
  connect(_player,SIGNAL(finished()),this,SLOT(clearDeadPlayers()));//,Qt::DirectConnection);
  //connect(_player,SIGNAL(terminated()),this,SLOT(clearDeadPlayers())); //,Qt::DirectConnection);
  _player->stop();

  _player = NULL;

#if 0 // Block below deadlocks occasionally
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
#endif
}


void
  fetch::ui::MainWindow::clearDeadPlayers()
{ while(!_dead_players.isEmpty())
  { IPlayerThread* p = _dead_players.dequeue();
    delete p;
  }
}