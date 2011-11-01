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

    QGridLayout *row;
    QDoubleSpinBox *s,**w;
    w=ps_;
    row = new QGridLayout();
    row->addWidget(new QLabel("X")   ,0,1,Qt::AlignCenter);
    row->addWidget(new QLabel("Y")   ,0,2,Qt::AlignCenter);
    row->addWidget(new QLabel("Z")   ,0,3,Qt::AlignCenter);
    row->addWidget(new QLabel("Step"),0,4,Qt::AlignCenter);

    row->addWidget(new QLabel("Pos (mm)"),1,0);
    row->addWidget(w[0]=parent->_stage_pos_x_control->createDoubleSpinBox(),1,1);
    row->addWidget(w[1]=parent->_stage_pos_y_control->createDoubleSpinBox(),1,2);
    row->addWidget(w[2]=parent->_stage_pos_z_control->createDoubleSpinBox(),1,3);    
    row->addWidget(   s=new QDoubleSpinBox(),1,4);
    for(int i=0;i<3;++i) w[i]->setDecimals(4);
    s->setRange(0.0001,10.0);    
    s->setDecimals(4);
    connect(s,SIGNAL(valueChanged(double)),this,SLOT(setPosStep(double)));
    s->setSingleStep(0.1);
    s->setValue(0.1);
    pstep_=s;
    
    w=vs_;
    row->addWidget(new QLabel("Pos (mm)"),2,0);
    row->addWidget(w[0]=parent->_stage_vel_x_control->createDoubleSpinBox(),2,1);
    row->addWidget(w[1]=parent->_stage_vel_y_control->createDoubleSpinBox(),2,2);
    row->addWidget(w[2]=parent->_stage_vel_z_control->createDoubleSpinBox(),2,3);
    row->addWidget(   s=new QDoubleSpinBox(),2,4);    
    for(int i=0;i<3;++i) w[i]->setDecimals(4);
    s->setRange(0.0001,10.0);    
    s->setDecimals(4);
    connect(s,SIGNAL(valueChanged(double)),this,SLOT(setVelStep(double)));
    s->setSingleStep(0.1);
    s->setValue(0.1);
    vstep_=s;

    form->addRow("Move",row);

    restoreSettings();
  }

  //void StageDockWidget::closeEvent(QCloseEvent* event)
  StageDockWidget::~StageDockWidget()
  { saveSettings();
  }

  void StageDockWidget::saveSettings()
  {
    QSettings settings;
    settings.setValue("StageDockWidget/vel/step",vstep_->value());
    settings.setValue("StageDockWidget/pos/step",pstep_->value());
  }

  void StageDockWidget::restoreSettings()
  { QSettings settings;
    vstep_->setValue(settings.value("StageDockWidget/vel/step",0.1).toDouble());
    pstep_->setValue(settings.value("StageDockWidget/pos/step",0.1).toDouble());
  }

  void StageDockWidget::setPosStep(double v)
  { for(int i=0;i<3;++i)
      ps_[i]->setSingleStep(v);
  }

  void StageDockWidget::setVelStep(double v)
  {for(int i=0;i<3;++i)
      vs_[i]->setSingleStep(v);
  }

//end namespace fetch::ui
}
}
