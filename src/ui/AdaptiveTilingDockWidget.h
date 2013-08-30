
#include <QtGui>
#include "devices/Microscope.h"
#include "AgentController.h"
#include "MainWindow.h"
namespace fetch{

  namespace ui{

    class AdaptiveTilingDockWidget:public QDockWidget
    {
    public:
      AdaptiveTilingDockWidget(device::Microscope *dc, MainWindow* parent);
    };

    //end namespace fetch::ui
  }
}
