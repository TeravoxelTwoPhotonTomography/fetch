#include "AgentController.h"
#include <assert.h>

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
      if(src==ATTACHED) emit onArm(agent_->_task);
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


}} //end fetch::ui
