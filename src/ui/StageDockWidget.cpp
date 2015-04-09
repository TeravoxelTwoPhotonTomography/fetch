#include "StageDockWidget.h"
#include "devices\microscope.h"
#include "ui\DevicePropController.h"

#define TRY(e) if(!(e)) qFatal("%s(%d):"ENDL "\tExpression evaluated as false"ENDL "\t%s",__FILE__,__LINE__,#e)

namespace fetch {
namespace ui {

  /*
   *    StageIndicator
   */
  
  StageIndicator::StageIndicator(device::Stage *dc,const QString& ontext, const QString& offtext, QWidget  *parent/*=NULL*/)
    : QLabel(parent),
      ontext_(ontext),
      offtext_(offtext),
      dc_(dc),
      last_(-1)
  { QTimer *t = new QTimer(this);
    connect(t,SIGNAL(timeout()),this,SLOT(poll()));
    t->start(100/*ms*/);
    setText("Unknown");
  }

  StageIndicator::~StageIndicator()
  { QTimer *t = findChild<QTimer *>();
    if(t) t->stop();
  }

  void StageIndicator::poll()
  { if( state() )
    { if(last_!=1)
        setText(ontext_);
      last_=1;
    } else
    { if(last_!=0)
        setText(offtext_);
      last_=0;
    }
  }


  /*
   *    StageMovingIndicator
   */
  struct StageMovingIndicator:public StageIndicator
  { StageMovingIndicator(device::Stage *dc,QWidget  *parent=NULL) : StageIndicator(dc,"Moving","Not Moving",parent) {}
    protected:
      bool state() {return dc_->isMoving();}
  };

  /*
   *    StageOnTargetIndicator
   */
  struct StageOnTargetIndicator:public StageIndicator
  { StageOnTargetIndicator(device::Stage *dc,QWidget  *parent=NULL) : StageIndicator(dc,"On Target","Off Target",parent) {}
    protected:
      bool state() {return dc_->isOnTarget();}
  };  
  
  /*
   *    StageServoStatusIndicator
   */
  struct StageServoStatusIndicator:public StageIndicator
  { StageServoStatusIndicator(device::Stage *dc,QWidget  *parent=NULL) : StageIndicator(dc,"Servo On","Servo Off",parent) {}
    protected:
      bool state() {return dc_->isServoOn();}
  };  
  
  /*
   *    StageReferencedIndicator
   */
  struct StageReferencedIndicator:public StageIndicator
  { StageReferencedIndicator(device::Stage *dc,QWidget  *parent=NULL) : StageIndicator(dc,"Ref OK","UNREF'D",parent) {}
    protected:
      bool state() {return dc_->isReferenced();}
  };

  /**
       \class StageBoundsButton
       
       Two state button:       
          -# (cleared) press to set target bound, goto (bound)
          -# (bound)   press to clear target bound, goto (cleared)
       
       \note outer bounds (biggest max and smallest min) should be set on the target \ref QDouble.
       
       Example:
       \code
          StageBoundsButton(ps_[0],"maximum",this)
       \endcode
       
   */
  StageBoundsButton::StageBoundsButton(QDoubleSpinBox *target, const char* targetprop, QWidget *parent)
      : QPushButton(parent)
      , target_(target)
      , targetprop_(targetprop)
  { 
    setCheckable(true);
    setChecked(false);
    TRY(connect(this,SIGNAL(toggled(bool)),this,SLOT(handleToggled(bool))));
    
    bool ok;
    original_ = target_->property(targetprop_).toDouble(&ok);
    TRY(ok);
    setText(QString::number(original_,'f',4));
  }

  void StageBoundsButton::handleToggled(bool state)
  { // state: false is cleared, true is bound
    if(state) // set to bound
    { double pos = target_->value();         
      target_->setProperty(targetprop_,pos);                  // set targetprop ("maximum" or "minimum") to current pos
      setText(QString::number(pos,'f',4));                    // button text reflects set point
    } else    // set to cleared
    { setText(QString::number(original_,'f',4));
      target_->setProperty(targetprop_,original_);            // set targetprop ("maximum" or "minimum") to original_
    }
  }
  

  //////////////////////////////////////////////////////////////////////////
  ///  \class  StageDockWidget
  ///
  ///   A QDockWidget for controlling the 3D stage sytem responsible 
  ///   for positioning the sample.
  //////////////////////////////////////////////////////////////////////////
  
  /** \param[in] dc     Microscope device context.
      \param[in] parent Must have the MainWindow type because the MainWindow
                        has the stage controller instance.
  */
  StageDockWidget::StageDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Stage",parent)
    ,dc_(dc)
    ,pstep_(0)
    ,vstep_(0)
  { 
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);

    QGridLayout *row;
    int irow = 0;
    QDoubleSpinBox *s,**w;
    ///// Position
    w=ps_;
    w[0]=parent->_stage_pos_x_control->createDoubleSpinBox();
    w[1]=parent->_stage_pos_y_control->createDoubleSpinBox();
    w[2]=parent->_stage_pos_z_control->createDoubleSpinBox();
    w[0]->setRange(0.5,100.0); // set safe ranges
    w[1]->setRange(0.5,100.0); // - normally this should be handled by DevicePropController
    w[2]->setRange(0.5,45.0);  //   but API's not right...validator doesn't accomidate non-lineedit controls well
                               //   need to explicitly set min-max or something    

    row = new QGridLayout();
    row->addWidget(new QLabel("X")   ,irow,1,Qt::AlignCenter);
    row->addWidget(new QLabel("Y")   ,irow,2,Qt::AlignCenter);
    row->addWidget(new QLabel("Z")   ,irow,3,Qt::AlignCenter);
    row->addWidget(new QLabel("Step"),irow,4,Qt::AlignCenter);
    irow++;

    ///// Max bounds
    row->addWidget(new QLabel("Max"),irow,0,Qt::AlignRight);
    row->addWidget(new StageBoundsButton(w[0],"maximum",this),irow,1);
    row->addWidget(new StageBoundsButton(w[1],"maximum",this),irow,2);
    row->addWidget(new StageBoundsButton(w[2],"maximum",this),irow,3);
    irow++;

    ///// Position spin boxes
    row->addWidget(new QLabel("Pos (mm)"),irow,0);
    row->addWidget(w[0],irow,1);
    row->addWidget(w[1],irow,2);
    row->addWidget(w[2],irow,3);    
    row->addWidget(   s=new QDoubleSpinBox(),irow,4); // step size 
    irow++;

    ///// Min bounds
    row->addWidget(new QLabel("Min"),irow,0,Qt::AlignRight);
    row->addWidget(new StageBoundsButton(w[0],"minimum",this),irow,1);
    row->addWidget(new StageBoundsButton(w[1],"minimum",this),irow,2);
    row->addWidget(new StageBoundsButton(w[2],"minimum",this),irow,3);
    irow++;

    for(int i=0;i<3;++i) w[i]->setDecimals(4);
    s->setRange(0.0001,10.0);    
    s->setDecimals(4);
    connect(s,SIGNAL(valueChanged(double)),this,SLOT(setPosStep(double)));
    s->setSingleStep(0.1);
    s->setValue(0.1);
    pstep_=s;

    ///// Lock Controls
    QCheckBox *b = new QCheckBox();        
    QStateMachine *lockmachine = new QStateMachine(this);
    QState *locked = new QState(),
         *unlocked = new QState();
    locked->addTransition(b,SIGNAL(stateChanged(int)),unlocked);
    unlocked->addTransition(b,SIGNAL(stateChanged(int)),locked);
    locked->assignProperty(b  ,"text","Locked");
    unlocked->assignProperty(b,"text","Unlocked");
    for(int i=0;i<3;++i)
    { locked->assignProperty(w[i]  ,"readOnly",true);
      unlocked->assignProperty(w[i],"readOnly",false);
    }
    lockmachine->addState(locked);
    lockmachine->addState(unlocked);
    b->setCheckState(Qt::Checked);
    lockmachine->setInitialState(locked);   
    lockmachine->start();

    ///// Velocity
    w=vs_;
    row->addWidget(new QLabel("Vel (mm/s)"),irow,0);
    row->addWidget(w[0]=parent->_stage_vel_x_control->createDoubleSpinBox(),irow,1);
    row->addWidget(w[1]=parent->_stage_vel_y_control->createDoubleSpinBox(),irow,2);
    row->addWidget(w[2]=parent->_stage_vel_z_control->createDoubleSpinBox(),irow,3);
    w[0]->setRange(0.1,10.0);  // set safe ranges
    w[1]->setRange(0.1,10.0);  // - normally this should be handled by DevicePropController
    w[2]->setRange(0.1,1.0);   //   but API's not right...validator doesn't accomidate non-lineedit controls well
                               //   need to explicitly set min-max or something
    
    row->addWidget(   s=new QDoubleSpinBox(),irow,4);
    irow++;
    for(int i=0;i<3;++i) w[i]->setDecimals(4);
    s->setRange(0.0001,10.0);    
    s->setDecimals(4);
    connect(s,SIGNAL(valueChanged(double)),this,SLOT(setVelStep(double)));
    s->setSingleStep(0.1);
    s->setValue(0.1);
    vstep_=s;
    form->addRow(row);
    form->addRow("Lock controls",b);

    ///// Indicators
    { QGridLayout *layout = new QGridLayout();
      layout->addWidget(new StageMovingIndicator(dc->stage())     ,0,0);
      layout->addWidget(new StageOnTargetIndicator(dc->stage())   ,0,1);
      layout->addWidget(new StageServoStatusIndicator(dc->stage()),0,2);
      layout->addWidget(new StageReferencedIndicator(dc->stage()) ,1,0);
      form->addRow("Indicators",layout);
    }    
    
    ///// History
    { QHBoxLayout *layout = new QHBoxLayout;
      layout->addWidget(parent->_stageController->createHistoryComboBox());
      QPushButton *b = new QPushButton("Add");
      connect(b,SIGNAL(clicked()),parent->_stageController,SLOT(savePosToHistory()));
      layout->addWidget(b);
      form->addRow("History", layout);
    }

    ///// Reference
    { 
      QPushButton *doit   = new QPushButton("Locked"),
                  *safety = new QPushButton("Unlock");
      QStateMachine    *s = new QStateMachine(this);
      QState      *locked = new QState(),
                *unlocked = new QState();
      locked->addTransition(  safety,SIGNAL(clicked()),unlocked);
      unlocked->addTransition(safety,SIGNAL(clicked()),locked);
      unlocked->addTransition(  doit,SIGNAL(clicked()),locked);    // re-lock after reference
      s->addState(locked);
      s->addState(unlocked);
      s->setInitialState(locked);
      s->start();
      
      locked->assignProperty(  doit,"text"   ,"Locked");
      locked->assignProperty(  doit,"enabled",false);
      unlocked->assignProperty(doit,"text"   ,"DO IT");
      unlocked->assignProperty(doit,"enabled",true);
      
      locked->assignProperty(  safety,"text" ,"Unlock");      
      unlocked->assignProperty(safety,"text" ,"Lock");
      
      QHBoxLayout *layout = new QHBoxLayout;
      layout->addWidget(safety);
      layout->addWidget(doit);
                
      connect(doit,SIGNAL(clicked()),parent->_stageController,SLOT(reference()));
      form->addRow("Reference:",layout);
    }
    
    ///// Error status
    { QPushButton *b = new QPushButton("Clear");
      connect(b,SIGNAL(clicked()),parent->_stageController,SLOT(clear()));
      form->addRow("Error status:",b);
    }

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

  void StageDockWidget::setPosStepByThickness()
  { pstep_->setValue(1e-3*(dc_->fov_.field_size_um_[2] - dc_->fov_.overlap_um_[2])); //um->mm
  }

  void StageDockWidget::setVelStep(double v)
  {for(int i=0;i<3;++i)
      vs_[i]->setSingleStep(v);
  }

//end namespace fetch::ui
}
}
