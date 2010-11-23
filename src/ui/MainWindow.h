#include <QtGui>

namespace fetch {
namespace device {class Microscope;}
namespace ui {

class VideoAcquisitionDockWidget;
class Figure;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(device::Microscope *dc);

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
};

//namespace ends
} //ui
} //fetch
