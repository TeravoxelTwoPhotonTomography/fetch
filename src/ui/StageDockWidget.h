#include <QtGui>
#include "MainWindow.h"

namespace fetch{
  namespace device{class Stage;}
  namespace ui{

    class StageDockWidget:public QDockWidget
    {      
      Q_OBJECT

      device::Stage *dc_;
    public:
      StageDockWidget(device::Stage *dc, MainWindow* parent);

    };

    //end namespace fetch::ui
  }
}
