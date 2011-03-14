#include "StackAcquisitionDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"

namespace fetch {
namespace ui {



  StackAcquisitionDockWidget::StackAcquisitionDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Stack Acquisition",parent)
  {
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);

    parent->_zpiezo_max_control->createLineEditAndAddToLayout(form);
    parent->_zpiezo_min_control->createLineEditAndAddToLayout(form);
    parent->_zpiezo_step_control->createLineEditAndAddToLayout(form);

    AgentControllerButtonPanel *btns = new AgentControllerButtonPanel(&parent->_scope_state_controller,&dc->stack_task);
    form->addRow(btns);

    connect(btns->controller(),SIGNAL(onRun()),this,SIGNAL(onRun()));
  }


//end namespace fetch::ui
}
}
