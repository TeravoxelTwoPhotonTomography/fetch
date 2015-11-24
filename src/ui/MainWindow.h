#pragma once
#include <QtWidgets>
#include "ui/AgentController.h"
#include "ui/StageController.h"
#include "DevicePropController.h"

namespace fetch {
namespace device {class Microscope;}
namespace ui {

class ColormapDockWidget;
class VideoAcquisitionDockWidget;
class StackAcquisitionDockWidget;
class MicroscopeStateDockWidget;
class MicroscopeTcpServerDockWidget;
class VibratomeDockWidget;
class VibratomeGeometryDockWidget;
class AutoTileDockWidget;
class StageDockWidget;
class SurfaceScanDockWidget;
class HistogramDockWidget;
class TimeSeriesDockWidget;
class Figure;
class IPlayerThread;

namespace internal { class mainwindow_defered_update; }

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
  void resetTripDetector();
  void resetPMTController();
  void startVideo();
  void stopVideo();

  int maybeSave();

  void clearDeadPlayers();

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

  ColormapDockWidget           *_cmapDockWidget;
  VideoAcquisitionDockWidget   *_videoAcquisitionDockWidget;
  StackAcquisitionDockWidget   *_stackAcquisitionDockWidget;
  MicroscopeStateDockWidget    *_microscopesStateDockWidget;
  MicroscopeTcpServerDockWidget*_microscopeTcpServer;
  VibratomeDockWidget          *_vibratomeDockWidget;
  VibratomeGeometryDockWidget  *_vibratomeGeometryDockWidget;
  QDockWidget                  *_cutTaskDockWidget;
  StageDockWidget              *_stageDockWidget;
  SurfaceScanDockWidget        *_surfaceScanDockWidget;
  AutoTileDockWidget           *_autoTileDockWidget;
  TimeSeriesDockWidget         *_timeSeriesDockWidget;
  HistogramDockWidget          *_histogramDockWidget;
  Figure                       *_display;
  IPlayerThread                *_player;

  QTimer _poller;
  AgentController               _scope_state_controller;
  PlanarStageController        *_stageController;

  // Property controllers
  ResonantTurnController       *_resonant_turn_controller;
  LinesController              *_vlines_controller;
  LSMVerticalRangeController   *_lsm_vert_range_controller;
  PockelsController            *_pockels_controllers[2];
  VibratomeAmplitudeController *_vibratome_amp_controller;
  VibratomeFeedDisController   *_vibratome_feed_distance_controller;
  VibratomeFeedVelController   *_vibratome_feed_velocity_controller;
  VibratomeFeedAxisController  *_vibratome_feed_axis_controller;
  VibratomeFeedPosXController  *_vibratome_feed_pos_x_controller;
  VibratomeFeedPosYController  *_vibratome_feed_pos_y_controller;
  VibratomeZOffsetController   *_vibratome_z_offset_controller;
  VibratomeThickController     *_vibratome_thick_controller;
  ZPiezoMaxController          *_zpiezo_max_control;
  ZPiezoMinController          *_zpiezo_min_control;
  ZPiezoStepController         *_zpiezo_step_control;
  StagePosXController          *_stage_pos_x_control;
  StagePosYController          *_stage_pos_y_control;
  StagePosZController          *_stage_pos_z_control;
  StageVelXController          *_stage_vel_x_control;
  StageVelYController          *_stage_vel_y_control;
  StageVelZController          *_stage_vel_z_control;
  FOVOverlapZController        *_fov_overlap_z_controller;

  AutoTileZOffController               *_autotile_zoffum_control;
  AutoTileZMaxController               *_autotile_zmaxmm_control;
  AutoTileTimeoutMsController          *_autotile_timeoutms_control;
  AutoTileChanController               *_autotile_chan_control;
  AutoTileIntensityThresholdController *_autotile_intensity_thresh_control;
  AutoTileAreaThresholdController      *_autotile_area_thresh_control;

  QFileSystemWatcher           *_config_watcher;

private:
  void update_active_config_location_(const QString& path);
  void load_settings_();
  void save_settings_();
  static const int version__ = 0;

  QSignalMapper _config_delayed_load_mapper;
  QTimer        _config_delayed_load_timer;
  internal::mainwindow_defered_update *_defered_update;

  QQueue<IPlayerThread*> _dead_players;
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

namespace internal
{
  // Helper that updates the microscope config from another thread (so GUI thread won't block)
  // then notifies the GUI when it's done.  See: MainWindow::openMicroscopeConfig
  class mainwindow_defered_update : public QThread
  {
    Q_OBJECT

    MainWindow *w_;
    cfg::device::Microscope cfg_;

  public:
    mainwindow_defered_update(MainWindow *w)
      : w_(w)
    {
      connect(this,SIGNAL(done()),w,SIGNAL(configUpdated()),Qt::DirectConnection);
    }

    void update(const fetch::cfg::device::Microscope& cfg)
    { cfg_.CopyFrom(cfg);
      start();
    }
  private:
    void run()
    { w_->_dc->set_config(cfg_);
      emit done();
    }

  signals:
    void done();
  };
}

//namespace ends
} //ui
} //fetch
