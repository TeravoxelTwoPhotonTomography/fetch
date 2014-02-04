
#include <QtGui>
#include "devices/Microscope.h"
#include "AgentController.h"
#include "MainWindow.h"
namespace fetch{

  

  namespace ui{

    class SurfaceFindDockWidget:public QDockWidget
    {
    public:
      SurfaceFindDockWidget(device::Microscope *dc, MainWindow* parent);
    };

    //end namespace fetch::ui
  }
}
