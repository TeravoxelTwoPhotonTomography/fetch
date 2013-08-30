#include "AdaptiveTilingDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"
#include "tasks\AdaptiveTiledAcquisition.h"

namespace fetch {
namespace ui {

  static task::microscope::AdaptiveTiledAcquisition task_;

  AdaptiveTilingDockWidget::AdaptiveTilingDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Adaptive Tiling",parent)
  {
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);
    AgentControllerButtonPanel *btns = new AgentControllerButtonPanel(&parent->_scope_state_controller,&task_);
    form->addRow(btns);
  }


//end namespace fetch::ui
}
}
