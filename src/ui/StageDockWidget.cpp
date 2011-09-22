#include "StageDockWidget.h"
#include "devices\microscope.h"
#include "ui\DevicePropController.h"

namespace fetch {
namespace ui {



  StageDockWidget::StageDockWidget(device::Stage *dc, MainWindow *parent)
    :QDockWidget("Stage",parent)
    ,dc_(dc)
  {
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);

    parent->_stage_pos_x_control->createDoubleSpinBoxAndAddToLayout(form);
    parent->_stage_pos_y_control->createDoubleSpinBoxAndAddToLayout(form);
    parent->_stage_pos_z_control->createDoubleSpinBoxAndAddToLayout(form);
    parent->_stage_vel_x_control->createDoubleSpinBoxAndAddToLayout(form);
    parent->_stage_vel_y_control->createDoubleSpinBoxAndAddToLayout(form);
    parent->_stage_vel_z_control->createDoubleSpinBoxAndAddToLayout(form);


  }


//end namespace fetch::ui
}
}
