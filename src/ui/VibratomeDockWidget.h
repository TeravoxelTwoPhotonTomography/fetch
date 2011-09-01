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
      QComboBox *cmb_feed_axis_;
    public:
      VibratomeDockWidget(device::Microscope *dc, MainWindow* parent);
    public slots:
      void start() {dc_->vibratome()->start();}
      void stop()  {dc_->vibratome()->stop();}
      void feedAxisChanged(int i);
    };

    //end namespace fetch::ui
  }
}
