#include "StageDockWidget.h"
#include "devices\microscope.h"
#include "ui\DevicePropController.h"

namespace fetch {
namespace ui {

  /*
   *    StageMovingIndicator
   */

  StageMovingIndicator::StageMovingIndicator(device::Stage *dc,QWidget  *parent)
    : QLabel(parent),
      dc_(dc),
      last_moving_(-1)
  { QTimer *t = new QTimer(this);
    connect(t,SIGNAL(timeout()),this,SLOT(poll()));
    t->start(100/*ms*/);
    setText("Maybe Moving");
  }

  StageMovingIndicator::~StageMovingIndicator()
  { QTimer *t = findChild<QTimer *>();
    if(t) t->stop();
  }

  void StageMovingIndicator::poll()
  { if( dc_->isMoving() )
    { if(last_moving_!=1)
        setText("Moving!");
      last_moving_=1;
    } else
    { if(last_moving_!=0)
        setText("Not Moving");
      last_moving_=0;
    }
  }

  /*
   *    StageOnTargetIndicator
   */

  StageOnTargetIndicator::StageOnTargetIndicator(device::Stage *dc,QWidget  *parent)
    : QLabel(parent),
      dc_(dc),
      last_ont_(-1)
  { QTimer *t = new QTimer(this);
    connect(t,SIGNAL(timeout()),this,SLOT(poll()));
    t->start(100/*ms*/);
    setText("Maybe on target");
  }

  StageOnTargetIndicator::~StageOnTargetIndicator()
  { QTimer *t = findChild<QTimer *>();
    if(t) t->stop();
  }

  void StageOnTargetIndicator::poll()
  { if( dc_->isOnTarget() )
    { if(last_ont_!=1)
        setText("On Target");
      last_ont_=1;
    } else
    { if(last_ont_!=0)
        setText("Not on Target.");
      last_ont_=0;
    }
  }

  /*
   *    StageDockWidget
   */

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
    row->addWidget(new QLabel("Vel (mm/s)"),2,0);
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

    form->addRow(row);

    form->addRow(new StageMovingIndicator(dc));
    form->addRow(new StageOnTargetIndicator(dc));

    form->addRow("History", parent->_stageController->createHistoryComboBox());    

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
