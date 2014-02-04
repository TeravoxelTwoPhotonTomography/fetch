#include "AgentController.h"
#include <assert.h>

#include <QtWidgets>

namespace fetch {
namespace ui {

AgentController::AgentController(Agent* agent)
  : agent_(agent)
  , runlevel_(UNKNOWN)
{
  assert(agent); 
}

void AgentController::poll()
{
  RunLevel current = queryRunLevel();
  if(current==runlevel_) return; // nothing to do.
  handleTransition(runlevel_,current);
  runlevel_=current;
}

void AgentController::handleTransition(AgentController::RunLevel src, AgentController::RunLevel dst)
{
  assert(dst!=UNKNOWN);

  if(src<dst && (dst-src)>1) // move up the runlevel chain
  {
    handleTransition(src,(RunLevel)(src+1));
    handleTransition((RunLevel)(src+1),dst);
    return;
  }
  if(src>dst && (src-dst)>1) // move down the runlevel chain
  {
    handleTransition(src,(RunLevel)(src-1));
    handleTransition((RunLevel)(src-1),dst);
    return;
  }
  //Below: either src-dst = 1 or dst-src = 1 
  if(src==UNKNOWN) return; // nothing to do
  switch(dst)
  {
    case OFF:
      if(src==ATTACHED) emit onDetach();
      else UNREACHABLE;
      break;

    case ATTACHED:
      if(src==OFF) emit onAttach();
      else if(src==ARMED) emit onDisarm();
      else UNREACHABLE;
      break;

    case ARMED:
      if(src==ATTACHED) 
      { emit onArm(agent_->_task);
        emit onArm();
      }
      else if(src==RUNNING) emit onStop();
      else UNREACHABLE;
      break;

    case RUNNING:
      if(src==ARMED) emit onRun();
      else UNREACHABLE;
      break;

    default: //UNKNOWN...nothing to do
      UNREACHABLE;
  } 
}

AgentController::RunLevel AgentController::queryRunLevel()
{
  if(agent_->is_running()) return RUNNING;
  if(agent_->is_armed()) return ARMED;
  if(agent_->is_attached()) return ATTACHED;
  return OFF;
}

void AgentController::attach()
{
  Guarded_Assert_WinErr(agent_->attach_nowait());
}

void AgentController::detach()
{
  Guarded_Assert_WinErr(agent_->detach_nowait());
}

void AgentController::arm( Task *t )
{
  Guarded_Assert_WinErr(agent_->arm_nowait(t,agent_->_owner,INFINITE));
}

void AgentController::disarm()
{
  Guarded_Assert_WinErr(agent_->disarm_nowait(INFINITE));
}

void AgentController::run()
{
  Guarded_Assert_WinErr(agent_->run_nowait());
}

void AgentController::stop()
{
  Guarded_Assert_WinErr(agent_->stop_nowait(INFINITE));
}

QTimer * AgentController::createPollingTimer()
{
  QTimer *poller = new QTimer(this);
  connect(poller,SIGNAL(timeout()),this,SLOT(poll()));
  return poller;
}

QDockWidget* AgentController::createTaskDockWidget(const QString & title, Task *task, QWidget* parent)
{ QDockWidget*      d = new QDockWidget(title,parent);
  QWidget*          w = new QWidget(d);  
  QFormLayout* layout = new QFormLayout;    
  layout->addRow(new AgentControllerButtonPanel(this,task));
  w->setLayout(layout);
  d->setWidget(w);
  return d;
}

/**
NOTE:
  
  I've disabled the attach and detach buttons (by not adding them to the widget layout).
  They were not tremendously useful.  When I was tempted to use them, usually it was better
  just to close and reopen the application.  With the buttons present, there was a chance 
  for a misclick.

  Perhaps one day, it will be prudent to remove associated code, but for now, they live on
  in purgatory.
**/
AgentControllerButtonPanel::AgentControllerButtonPanel(AgentController *ac, Task *task)
  : armTarget_(task)
  , ac_(ac)
{
    btnDetach = new QPushButton("Detach");
    btnAttach = new QPushButton("Attach");
    btnArm    = new QPushButton("Arm");
    btnDisarm = new QPushButton("Disarm");
    btnRun    = new QPushButton("Run");
    btnStop   = new QPushButton("Stop");

    QObject::connect(btnDetach,SIGNAL(clicked()),ac_,SLOT(detach()));
    QObject::connect(btnAttach,SIGNAL(clicked()),ac_,SLOT(attach()));
    QObject::connect(btnArm,SIGNAL(clicked()),this,SLOT(armTargetTask()));
    QObject::connect(btnDisarm,SIGNAL(clicked()),ac_,SLOT(disarm()));
    QObject::connect(btnRun,SIGNAL(clicked()),ac_,SLOT(run()));
    QObject::connect(btnStop,SIGNAL(clicked()),ac_,SLOT(stop()));


    //
    // State machine
    //     
    taskDetached = new QState;
    taskAttached = new QState;
    taskArmed    = new QState;
    taskArmedNonTarget = new QState;
    taskRunning  = new QState;

    taskDetached->addTransition(ac_,SIGNAL(onAttach()),taskAttached);
    taskAttached->addTransition(ac_,SIGNAL(onDetach()),taskDetached);

    connect(ac_,SIGNAL(onArm(Task*)),this,SLOT(onArmFilter(Task*)));
    taskAttached->addTransition(this,SIGNAL(onArmTargetTask()),taskArmed);
    taskAttached->addTransition(this,SIGNAL(onArmNonTargetTask()),taskArmedNonTarget);

    taskArmed->addTransition(ac_,SIGNAL(onDisarm()),taskAttached);
    taskArmed->addTransition(ac_,SIGNAL(onRun()),taskRunning);
    taskArmedNonTarget->addTransition(ac_,SIGNAL(onDisarm()),taskAttached);
    taskRunning->addTransition(ac_,SIGNAL(onStop()),taskArmed);

    stateMachine_.addState(taskArmed);
    stateMachine_.addState(taskArmedNonTarget);
    stateMachine_.addState(taskAttached);
    stateMachine_.addState(taskDetached);
    stateMachine_.addState(taskRunning);
    stateMachine_.setInitialState(taskDetached);
    stateMachine_.start();

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
      c->assignProperty(btnArm,   "enabled",armTarget_!=NULL);
      c->assignProperty(btnDisarm,"enabled",false);
      c->assignProperty(btnRun,   "enabled",false);
      c->assignProperty(btnStop,  "enabled",false);

      c = taskArmed;
      c->assignProperty(btnDetach,"enabled",true);
      c->assignProperty(btnAttach,"enabled",false);
      c->assignProperty(btnArm,   "enabled",false);
      c->assignProperty(btnDisarm,"enabled",true);
      c->assignProperty(btnRun,   "enabled",true);
      c->assignProperty(btnStop,  "enabled",true);

      c = taskArmedNonTarget;
      c->assignProperty(btnDetach,"enabled",true);
      c->assignProperty(btnAttach,"enabled",false);
      c->assignProperty(btnArm,   "enabled",false);
      c->assignProperty(btnDisarm,"enabled",true);
      c->assignProperty(btnRun,   "enabled",false);
      c->assignProperty(btnStop,  "enabled",false);

      c = taskRunning;
      c->assignProperty(btnDetach,"enabled",true);
      c->assignProperty(btnAttach,"enabled",false);
      c->assignProperty(btnArm,   "enabled",false);
      c->assignProperty(btnDisarm,"enabled",true);
      c->assignProperty(btnRun,   "enabled",false);
      c->assignProperty(btnStop,  "enabled",true);
    }

    QGridLayout *layout;
    layout = new QGridLayout;
    //layout->addWidget(btnAttach,0,0);
    layout->addWidget(btnArm,0,1);
    layout->addWidget(btnRun,0,2);
    
    //layout->addWidget(btnDetach,1,0);
    layout->addWidget(btnDisarm,1,1);
    layout->addWidget(btnStop,1,2);
    setLayout(layout);    
}

void AgentControllerButtonPanel::onArmFilter( Task* t )
{
  if(t==armTarget_)
    emit onArmTargetTask();
  else
    emit onArmNonTargetTask();
}

void AgentControllerButtonPanel::armTargetTask()
{
  if(armTarget_)
    emit ac_->arm(armTarget_);
}


}} //end fetch::ui
