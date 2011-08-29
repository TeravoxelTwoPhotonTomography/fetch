#include <QtGui>
#include "devices/Microscope.h"
#include "AgentController.h"
#include "MainWindow.h"
namespace fetch{

  namespace ui{

    class VibratomeDockWidget:public QDockWidget
    {      
      Q_OBJECT

      device::Microscope *dc_;
    public:
      VibratomeDockWidget(device::Microscope *dc, MainWindow* parent);
    public slots:
      void start() {dc_->vibratome()->start();}
      void stop() {dc_->vibratome()->stop();}
    };

    //end namespace fetch::ui
  }
}
