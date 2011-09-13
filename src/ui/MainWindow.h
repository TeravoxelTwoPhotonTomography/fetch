#pragma once
#include <QtGui>
#include "ui/AgentController.h"
#include "ui/StageController.h"
#include "DevicePropController.h"

namespace fetch {
namespace device {class Microscope;}
namespace ui {

class VideoAcquisitionDockWidget;
class StackAcquisitionDockWidget;
class MicroscopeStateDockWidget;
class VibratomeDockWidget;
class VibratomeGeometryDockWidget;
class Figure;
class IPlayerThread;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(device::Microscope *dc);
  virtual ~MainWindow();

  TilingController *tilingController() {return _stageController->tiling();}

  static const char defaultConfigPathKey[];       

signals:
  void configUpdated();
  void configFileChanged(const QString& filename);

protected:
  void closeEvent(QCloseEvent *event); 

protected slots:
  void openMicroscopeConfigViaFileDialog();
  void openMicroscopeConfigDelayed(const QString& filename);
  void openMicroscopeConfig(const QString& filename);
  void saveMicroscopeConfigViaFileDialog();
  void saveMicroscopeConfigToLastGoodLocation();
  void saveMicroscopeConfig(const QString& filename);

  void startVideo();
  void stopVideo(); 

public: // semi-private
  void createStateMachines();
  void createMenus();
  void createActions();
  void createDockWidgets();
  void createViews();
  device::Microscope *_dc;

  QStateMachine fullscreenStateMachine;
  QMenu   *viewMenu;
  QMenu   *fileMenu;

  QAction *quitAct;
  QAction *openAct;
  QAction *saveAct;
  QAction *saveToAct;

  QAction *fullscreenAct;
  QState  *fullscreenStateOn;
  QState  *fullscreenStateOff;

  VideoAcquisitionDockWidget   *_videoAcquisitionDockWidget;
  StackAcquisitionDockWidget   *_stackAcquisitionDockWidget;
  MicroscopeStateDockWidget    *_microscopesStateDockWidget;
  VibratomeDockWidget          *_vibratomeDockWidget;
  VibratomeGeometryDockWidget  *_vibratomeGeometryDockWidget;
  Figure                       *_display;
  IPlayerThread                *_player;
                                                             
  QTimer _poller;                                            
  AgentController               _scope_state_controller;
  PlanarStageController        *_stageController;
  
  // Property controllers
  ResonantTurnController       *_resonant_turn_controller;
  LinesController              *_vlines_controller;
  LSMVerticalRangeController   *_lsm_vert_range_controller;
  PockelsController            *_pockels_controller;
  VibratomeAmplitudeController *_vibratome_amp_controller;
  VibratomeFeedDisController   *_vibratome_feed_distance_controller;
  VibratomeFeedVelController   *_vibratome_feed_velocity_controller;
  VibratomeFeedAxisController  *_vibratome_feed_axis_controller;
  ZPiezoMaxController          *_zpiezo_max_control;
  ZPiezoMinController          *_zpiezo_min_control;
  ZPiezoStepController         *_zpiezo_step_control;

  QFileSystemWatcher           *_config_watcher;

private:  
  void update_active_config_location_(const QString& path);
  void load_settings_();
  void save_settings_();
  static const int version__ = 0;

  QSignalMapper _config_delayed_load_mapper;
  QTimer        _config_delayed_load_timer;
};


class ConfigFileNameDisplay : public QLabel
{ Q_OBJECT

  public:
    ConfigFileNameDisplay(const QString& filename, QWidget *parent=0);

  public slots:
    void update(const QString& filename);
};

class FileSeriesNameListener : public QObject, public device::FileSeriesListener
{ Q_OBJECT
public:
  virtual void update(const std::string& path)
  { emit sig_update(QString(path.c_str()));
  }
signals:
  void sig_update(const QString& path);
};

class FileSeriesNameDisplay : public QLabel
{ Q_OBJECT

  public:
    FileSeriesNameDisplay(const QString& filename, QWidget *parent=0);

    inline FileSeriesNameListener* listener() {return &_listener;}

  public slots:
    void update(const QString& filename);

  private:
    FileSeriesNameListener _listener;

};

//namespace ends
} //ui
} //fetch
