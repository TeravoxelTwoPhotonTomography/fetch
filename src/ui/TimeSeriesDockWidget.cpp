#include "TimeSeriesDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"

namespace fetch {
namespace ui {



  TimeSeriesDockWidget::TimeSeriesDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Time series",parent)
  {
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);
    AgentControllerButtonPanel *btns = new AgentControllerButtonPanel(&parent->_scope_state_controller,&dc->time_series_task);
    form->addRow(btns);
  }


//end namespace fetch::ui
}
}
