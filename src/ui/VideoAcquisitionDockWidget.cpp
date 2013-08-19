#include "VideoAcquisitionDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"

namespace fetch {
namespace ui {



  VideoAcquisitionDockWidget::VideoAcquisitionDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Video Acquisition",parent)
  {
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);

    parent->_resonant_turn_controller->createLineEditAndAddToLayout(form);
    parent->_vlines_controller->createLineEditAndAddToLayout(form);
    parent->_lsm_vert_range_controller->createLineEditAndAddToLayout(form);
    parent->_pockels_controllers[0]->createLineEditAndAddToLayout(form);
    parent->_pockels_controllers[1]->createLineEditAndAddToLayout(form);

    AgentControllerButtonPanel *btns = new AgentControllerButtonPanel(&parent->_scope_state_controller,&dc->interaction_task);
    form->addRow(btns);      

    connect(btns->controller(),SIGNAL(onRun()),this,SIGNAL(onRun()));
  }


//end namespace fetch::ui
}
}
