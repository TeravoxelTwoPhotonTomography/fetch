#include "SurfaceFindDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"
#include "tasks\SurfaceFind.h"

namespace fetch {
namespace ui {

  static fetch::task::microscope::SurfaceFind task_;

  SurfaceFindDockWidget::SurfaceFindDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Surface find",parent)
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
