// [ ] TODO: REFACTOR: SurfaceScanDockWidget to TilingDockWidget

#include <QtWidgets>
#include "devices/Microscope.h"
#include "AgentController.h"
#include "MainWindow.h"
namespace fetch{

  

  namespace ui{

    class SurfaceScanDockWidget:public QDockWidget
    {
    public:
      SurfaceScanDockWidget(device::Microscope *dc, MainWindow* parent);
    };

    //end namespace fetch::ui
  }
}
