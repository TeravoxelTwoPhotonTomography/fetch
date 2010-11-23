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
      void onArm();
      void onDisarm();
      void onRun();
      void onStop();

    protected slots:
      void poll();
        
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

}} //end fetch::ui
