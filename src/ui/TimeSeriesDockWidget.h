
#include <QtWidgets>
#include "devices/Microscope.h"
#include "AgentController.h"
#include "MainWindow.h"
namespace fetch{

  namespace ui{

    class TimeSeriesDockWidget:public QDockWidget
    {
    public:
      TimeSeriesDockWidget(device::Microscope *dc, MainWindow* parent);
    };

    //end namespace fetch::ui
  }
}
