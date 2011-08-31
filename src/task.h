/* Task.h
 *
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 20, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once

#include "agent.h"

/* Task
 * ----
 *
 * A Task instance should not carry any permanent state.  This class defines an
 * associated set of callbacks used by an Agent.
 *
 * When a Task instance is not part of a running Agent, it has no guaranteed
 * lifetime.
 *
 * The Task::config() function is called by the Agent::arm() function and is
 * responsible for setting up resources for the Task::run() function.
 * Task::run() is called by the Task::thread_main() function when an
 * associated Agent is transitioned to the Run state.  Task::run() acts like
 * the task's main loop and should terminate when the Agent::notify_stop event
 * is set, if not before.
 *
 * The Agent state model guarantees calls to Task::run() will follow calls to
 * Task::config().  However, config() may be called several times (by cycling
 * Agent arm's and disarm's) before run() is called.  Persistent resources
 * used by a task should be allocated in the Agent during the attach/detach
 * routines.
 *
 * Notes on "Cast" tasks
 * ---------------------
 * 
 * Historical.  The code of interest here has been removed.
 * 
 * The Task interface is defined over abstract Agents.  In order to support
 * runtime type checking of the interface for Agent children, a castable
 * interface can be written using templates.  The config and run functions
 * each dynamically upcast to the child and call another config or run 
 * function with the correct type.
 *
 * There's a problem with this.  The Task::eq relation is broken with this
 * technique.  That is, all classes deriving from UpcastTask are equivilent.
 * This effectively makes the Task::eq useless for discriminating between
 * task types.
 *
 * Discriminating between tasks is useful when one wants to compare the 
 * armed task against a list of other tasks.  This happens in the user 
 * interface, for example, to switch tasks.
 *
 * So casting tasks should be avoided.  The solution is to do the runtime
 * cast in each of the child classes explicitly.  This adds repetitive
 * code but supports Task::eq.
 *
 */

#ifndef WINAPI
#define WINAPI
#endif

namespace fetch
{
  //typedef void Agent;
  class Agent;
  class IDevice;

  class Task
  { public:
    virtual unsigned int config(IDevice *d) = 0; // return 1 on success, 0 otherwise
    virtual unsigned int run(IDevice *d)    = 0; // return 1 on success, 0 otherwise
  
    static DWORD WINAPI thread_main(LPVOID lpParam);

    static bool eq(Task *a, Task *b); // a eq b iff addresses of config and run virtual functions share the same address
  };

  template<typename TDevice>
    class TTask : public Task
  { public:
      virtual unsigned int config(IDevice *d) {return config(dynamic_cast<TDevice*>(d));}
      virtual unsigned int run(IDevice *d)    {return    run(dynamic_cast<TDevice*>(d));}
      virtual unsigned int config(TDevice *d) {return 1;}
      virtual unsigned int run(TDevice *d)    = 0;
  };
  
  /*
     IUpdateable
     --------------
    
     An additional update() function is provided to supplement the config()
     function.
    
     config() is intended to be used to set up the entire set of resources
     required for a task.  However, in some applications, a subset of those
     resources are frequently updated.  The update() function provided here
     allows one to implement less expensive updates for frequently changed
     resources.
    
     update() is only called while the associated Agent is in an Armed state.
   */
  class IUpdateable
  { public:
    virtual unsigned int update(IDevice *d) = 0; //return 1 on success, 0 otherwise
  };
  
}
