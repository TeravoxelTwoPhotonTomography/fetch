#include <QtWidgets>
#include "devices/Microscope.h"
#include "AgentController.h"
#include "MainWindow.h"
namespace fetch{

  namespace ui{

    class MicroscopeTcpServerDockWidget:public QDockWidget
    {
    public:
        MicroscopeTcpServerDockWidget(device::Microscope* dc,AgentController *ac,MainWindow* parent);
    };

    //end namespace fetch::ui
  }
}
