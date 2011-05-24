#include "MicroscopeStateDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"

namespace fetch {
namespace ui {



  MicroscopeStateDockWidget::MicroscopeStateDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Tiling",parent)
  {
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);
    AgentControllerButtonPanel *btns = new AgentControllerButtonPanel(&parent->_scope_state_controller,&dc->tiling_task);
    form->addRow(btns);
  }


//end namespace fetch::ui
}
}
