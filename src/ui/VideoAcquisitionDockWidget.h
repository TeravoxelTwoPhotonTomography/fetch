#include <QtWidgets>
#include "devices/Microscope.h"
#include "AgentController.h"
#include "MainWindow.h"
namespace fetch{

  namespace ui{

    class VideoAcquisitionDockWidget:public QDockWidget
    {
      Q_OBJECT

    public:
      typedef device::Microscope::Config Config;

      VideoAcquisitionDockWidget(device::Microscope *dc, MainWindow* parent);
        
    signals:
      void onRun();

    //private:      
    //  void createForm();
    };

    //end namespace fetch::ui
  }
}
