#include <QtGui>

namespace fetch {
namespace ui {

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();

private slots:
  void fullscreen();

private:
  void createMenus();
  void createActions();

  QMenu   *viewMenu;

  QAction *fullscreenAct;
};

//namespace ends
} //ui
} //fetch