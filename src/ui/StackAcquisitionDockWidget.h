#include <QtGui>
#include "devices/Microscope.h"
#include "AgentController.h"
#include "MainWindow.h"
namespace fetch{

  

  namespace ui{

    class StackAcquisitionDockWidget:public QDockWidget
    {
      Q_OBJECT

    public:
      typedef device::Microscope::Config Config;

      StackAcquisitionDockWidget(device::Microscope *dc, MainWindow* parent);


    private:      
      void createForm();
    };

    //end namespace fetch::ui
  }
}
