#pragma once
#include <QtGui>
#include <agent.h>

namespace fetch {
namespace ui {

  class AgentController:public QObject
  { 
    Q_OBJECT
    public:
      AgentController(Agent *agent);

    signals:
      void onAttach();
      void onDetach();
      void onArm(Task *t);
      void onDisarm();
      void onRun();
      void onStop();

    protected slots:
      void poll();

    public slots:
      void attach();
      void detach();
      void arm(Task *t);
      void disarm();
      void run();
      void stop();
        
    protected:
      Agent* agent_;
      int    poll_interval_ms_;

      enum RunLevel
      {
        UNKNOWN = -1,
        OFF = 0,        
        ATTACHED,
        ARMED,
        RUNNING
      } runlevel_;

      RunLevel queryRunLevel();
      void handleTransition(RunLevel src, RunLevel dst);
  };

  class AgentControllerButtonPanel : public QWidget
  {
    Q_OBJECT

    QStateMachine     stateMachine_;
    Task             *armTarget_;
    AgentController  *ac_;
  public:
    AgentControllerButtonPanel(AgentController *ac, Task *task=NULL);

    inline AgentController* controller() {return ac_;}

  public:
    QState 
      *taskDetached,
      *taskAttached,
      *taskArmed,
      *taskArmedNonTarget,
      *taskRunning;
    QPushButton 
      *btnDetach,
      *btnAttach,
      *btnArm,
      *btnDisarm,
      *btnRun,
      *btnStop;

  signals:
      void onArmTargetTask();
      void onArmNonTargetTask();
  public slots:
    void onArmFilter(Task* t);
    void armTargetTask();
  };

}} //end fetch::ui
