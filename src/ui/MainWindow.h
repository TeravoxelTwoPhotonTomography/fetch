#include <QtGui>
#include "ui/AgentController.h"

namespace fetch {
namespace device {class Microscope;}
namespace ui {

class VideoAcquisitionDockWidget;
class Figure;
class IPlayerThread;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(device::Microscope *dc);
  virtual ~MainWindow();

protected:
  void closeEvent(QCloseEvent *event);

private:
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
  QAction *saveToAct;

  QAction *fullscreenAct;
  QState  *fullscreenStateOn;
  QState  *fullscreenStateOff;

  VideoAcquisitionDockWidget *_videoAcquisitionDockWidget;
  Figure *_display;
  IPlayerThread *_player;
  
  QTimer _poller;
  AgentController _scope_state_broadcast;  
};

//namespace ends
} //ui
} //fetch
