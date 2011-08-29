#include "VibratomeDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"

namespace fetch {
namespace ui {



  VibratomeDockWidget::VibratomeDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Vibratome",parent)
    ,dc_(dc)
  {
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);

    parent->_vibratome_controller->createLineEditAndAddToLayout(form);            
    
    QHBoxLayout *row = new QHBoxLayout();
    
    QPushButton *b = new QPushButton("&Start",this);
    connect(b,SIGNAL(clicked()),this,SLOT(start()));
    row->addWidget(b);

    b = new QPushButton("Sto&p",this);
    connect(b,SIGNAL(clicked()),this,SLOT(stop()));
    row->addWidget(b);

    form->addRow(row);
  }


//end namespace fetch::ui
}
}
