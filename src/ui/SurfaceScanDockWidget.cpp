#include "SurfaceScanDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"

namespace fetch {
namespace ui {



  SurfaceScanDockWidget::SurfaceScanDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Surface scan",parent)
  {
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);
    AgentControllerButtonPanel *btns = new AgentControllerButtonPanel(&parent->_scope_state_controller,&dc->surface_scan_task);
    form->addRow(btns);
  }


//end namespace fetch::ui
}
}
