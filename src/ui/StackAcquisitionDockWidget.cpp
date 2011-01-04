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

    parent->_resonant_turn_controller->createLineEditAndAddToLayout(form);
    parent->_vlines_controller->createLineEditAndAddToLayout(form);
    parent->_lsm_vert_range_controller->createLineEditAndAddToLayout(form);
    parent->_pockels_controller->createLineEditAndAddToLayout(form);

    AgentControllerButtonPanel *btns = new AgentControllerButtonPanel(&parent->_scope_state_controller,&dc->stack_task);
    form->addRow(btns);
  }


//end namespace fetch::ui
}
}
