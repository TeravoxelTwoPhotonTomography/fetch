#include "VideoAcquisitionDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"

namespace fetch {
namespace ui {

  class EvenIntValidator:public QIntValidator
  {
  public:
    EvenIntValidator(int min, int max, QObject *parent) : QIntValidator(min,max,parent) {}
    virtual State validate(QString &in, int &pos) const
    {
      State state = QIntValidator::validate(in,pos);
      if(state!=Acceptable)
        return state;
      else {
        bool ok;
        unsigned v = in.toUInt(&ok);
        if(ok)
          if(v%2)
            return QValidator::Intermediate;
          else
            return state;
        return QValidator::Invalid;
      }
    }
    virtual void fixup(QString &input) const
    {
      bool ok;
      unsigned v = input.toUInt(&ok);
      if(ok)
      {
        if(v%2) // odd
          input = QString().setNum(v+1);
      }
    }
  };

  VideoAcquisitionDockWidget::VideoAcquisitionDockWidget(device::Microscope *dc, AgentController *ac, QWidget *parent/*=NULL*/)
    :QDockWidget("Video Acquisition",parent)
    ,_dc(dc)
    ,_ac(ac)
  {
    createForm();
    
  }

  void VideoAcquisitionDockWidget::setTurn()
  {   
    HERE;
    Config c = _dc->get_config();
    bool ok=0;    
    double px = _leResonantTurn->text().toDouble(&ok);
    if(!ok) goto ConversionFailed;
        
    c.mutable_resonant_wrap()->set_turn_px(px);
    Guarded_Assert(_dc->set_config_nowait(c));

    return;
ConversionFailed:
    warning("Couldn't interpret contents of line edit control.\r\n");
    return;   
  }

  void VideoAcquisitionDockWidget::setLines()
  {
    HERE;
    Config c = _dc->get_config();
    bool ok=0;    
    int nlines = _leLines->text().toInt(&ok);
    if(!ok) goto ConversionFailed;

    c.mutable_scanner3d()->mutable_scanner2d()->set_nscans(nlines/2);
    Guarded_Assert(_dc->set_config_nowait(c));

    return;
ConversionFailed:
    warning("Couldn't interpret contents of line edit control.\r\n");
    return;
  }

  void VideoAcquisitionDockWidget::setVerticalRange()
  {
    HERE;    
    bool ok=0;    
    double v = _leVerticalRange->text().toDouble(&ok);
    if(!ok) goto ConversionFailed;

    device::LinearScanMirror *p = _dc->LSM();
    //p->setAmplitudeVoltsNoWait(v);
    p->setAmplitudeVolts(v);

    return;
ConversionFailed:
    warning("Couldn't interpret contents of line edit control.\r\n");
    return;
  }

  void VideoAcquisitionDockWidget::setPockels()
  {
    HERE;    
    bool ok=0;
    int v = _lePockels->text().toInt(&ok);
    if(!ok) goto ConversionFailed;

    device::Pockels *p = _dc->pockels();
    p->setOpenVoltsNoWait(v/1000.0);

    return;
ConversionFailed:
    warning("Couldn't interpret contents of line edit control.\r\n");
    return;
  }

  void VideoAcquisitionDockWidget::createForm()
  {
    Config c = _dc->get_config();
    _leResonantTurn     = new QLineEdit(QString().setNum(c.resonant_wrap().turn_px(),'f',1));
    //_leDuty           = new QLineEdit;
    _leLines            = new QLineEdit(QString().setNum(c.scanner3d().scanner2d().nscans()*2));
    _leVerticalRange    = new QLineEdit(QString().setNum(_dc->LSM()->getAmplitudeVolts()));
    _lePockels          = new QLineEdit(QString().setNum(_dc->pockels()->getOpenVolts()*1000));
    _btnFocus           = new QPushButton("Focus");

    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    form->addRow("&Turn (px)",_leResonantTurn);
    form->addRow("&Lines (px)",_leLines);
    form->addRow("&Y Range (Vpp)",_leVerticalRange);
    form->addRow("&Pockels (mV)",_lePockels);
    form->addRow(_btnFocus);
    
    QPushButton 
      *btnDetach = new QPushButton("Detach"),
      *btnAttach = new QPushButton("Attach"),
      *btnArm    = new QPushButton("Arm"),
      *btnDisarm = new QPushButton("Disarm"),
      *btnRun    = new QPushButton("Run"),
      *btnStop   = new QPushButton("Stop");
    QObject::connect(btnDetach,SIGNAL(clicked()),_ac,SLOT(detach()));
    QObject::connect(btnAttach,SIGNAL(clicked()),_ac,SLOT(attach()));
    QObject::connect(btnArm,SIGNAL(clicked()),this,SLOT(armVideoTask()));
    QObject::connect(btnDisarm,SIGNAL(clicked()),_ac,SLOT(disarm()));
    QObject::connect(btnRun,SIGNAL(clicked()),_ac,SLOT(run()));
    QObject::connect(btnStop,SIGNAL(clicked()),_ac,SLOT(stop()));
    QHBoxLayout *layout;
    layout = new QHBoxLayout;
    layout->addWidget(btnAttach);
    layout->addWidget(btnArm);
    layout->addWidget(btnRun);
    form->addRow(layout);
    layout = new QHBoxLayout;
    layout->addWidget(btnDetach);
    layout->addWidget(btnDisarm);
    layout->addWidget(btnStop);
    form->addRow(layout);

    formwidget->setLayout(form);
    setWidget(formwidget);

    _leResonantTurn->setValidator( new QDoubleValidator (0.0, 2048.0, 1, formwidget));
    _leLines->setValidator(        new  EvenIntValidator(2,   4096,      formwidget));
    _leVerticalRange->setValidator(new QDoubleValidator (0.0, 20.0,   3, formwidget));
    _lePockels->setValidator(      new QIntValidator    (0,   2000,      formwidget));

    connect(_leResonantTurn,SIGNAL(editingFinished()),this,SLOT(setTurn()));
    connect(_leLines,SIGNAL(editingFinished()),this,SLOT(setLines()));
    connect(_leVerticalRange,SIGNAL(editingFinished()),this,SLOT(setVerticalRange()));
    connect(_lePockels,SIGNAL(editingFinished()),this,SLOT(setPockels()));

    //
    // State machine
    //
    QState 
      *taskDetached = new QState,
      *taskAttached = new QState,
      *taskArmed    = new QState,
      *taskRunning  = new QState;

    taskDetached->addTransition(_ac,SIGNAL(onAttach()),taskAttached);
    taskAttached->addTransition(_ac,SIGNAL(onDetach()),taskDetached);

    connect(_ac,SIGNAL(onArm(Task*)),this,SLOT(onArmFilter(Task*)));
    taskAttached->addTransition(this,SIGNAL(onArmVideoTask()),taskArmed);

    taskArmed->addTransition(_ac,SIGNAL(onDisarm()),taskAttached);
    taskArmed->addTransition(_ac,SIGNAL(onRun()),taskRunning);
    taskRunning->addTransition(_ac,SIGNAL(onStop()),taskArmed);

    _focusButtonStateMachine.addState(taskArmed);
    _focusButtonStateMachine.addState(taskAttached);
    _focusButtonStateMachine.addState(taskDetached);
    _focusButtonStateMachine.addState(taskRunning);
    _focusButtonStateMachine.setInitialState(taskDetached);
    _focusButtonStateMachine.start();

    taskDetached->assignProperty(_btnFocus,"text","Attach");
    taskAttached->assignProperty(_btnFocus,"text","Arm");
    taskArmed->assignProperty(_btnFocus,"text","Go");
    taskRunning->assignProperty(_btnFocus,"text","Stop");
    {
      QState *c = taskDetached;
      c->assignProperty(btnDetach,"enabled",false);
      c->assignProperty(btnAttach,"enabled",true);
      c->assignProperty(btnArm,   "enabled",false);
      c->assignProperty(btnDisarm,"enabled",false);
      c->assignProperty(btnRun,   "enabled",false);
      c->assignProperty(btnStop,  "enabled",false);

      c = taskAttached;
      c->assignProperty(btnDetach,"enabled",true);
      c->assignProperty(btnAttach,"enabled",false);
      c->assignProperty(btnArm,   "enabled",true);
      c->assignProperty(btnDisarm,"enabled",false);
      c->assignProperty(btnRun,   "enabled",false);
      c->assignProperty(btnStop,  "enabled",false);

      c = taskArmed;
      c->assignProperty(btnDetach,"enabled",true);
      c->assignProperty(btnAttach,"enabled",false);
      c->assignProperty(btnArm,   "enabled",false);
      c->assignProperty(btnDisarm,"enabled",true);
      c->assignProperty(btnRun,   "enabled",true);
      c->assignProperty(btnStop,  "enabled",false);

      c = taskRunning;
      c->assignProperty(btnDetach,"enabled",true);
      c->assignProperty(btnAttach,"enabled",false);
      c->assignProperty(btnArm,   "enabled",false);
      c->assignProperty(btnDisarm,"enabled",true);
      c->assignProperty(btnRun,   "enabled",false);
      c->assignProperty(btnStop,  "enabled",true);

    }

  }

  void VideoAcquisitionDockWidget::onArmFilter( Task* t )
  {
    if(t==&_dc->interaction_task)
      emit onArmVideoTask();
  }

  void VideoAcquisitionDockWidget::armVideoTask()
  {
    emit _ac->arm(&_dc->interaction_task);
  }

//end namespace fetch::ui
}
}
