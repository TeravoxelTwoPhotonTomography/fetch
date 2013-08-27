// [ ] TODO: REFACTOR: MicroscopeStateDockWidget to TilingDockWidget

#include <QtWidgets>
#include "devices/Microscope.h"
#include "AgentController.h"
#include "MainWindow.h"
namespace fetch{

  

  namespace ui{

    class MicroscopeStateDockWidget:public QDockWidget
    {
    public:
      MicroscopeStateDockWidget(device::Microscope *dc, MainWindow* parent);
    };

    //end namespace fetch::ui
  }
}
