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
#include "stdafx.h"
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
 */

#ifndef WINAPI
#define WINAPI
#endif

namespace fetch
{
  //typedef void Agent;
  class Agent;

  class Task
  { public:
    virtual unsigned int config(Agent *d) = 0; // return 1 on success, 0 otherwise
    virtual unsigned int run(Agent *d)    = 0; // return 1 on success, 0 otherwise
  
    static DWORD WINAPI thread_main(LPVOID lpParam);

    static bool eq(Task *a, Task *b); // a eq b iff addresses of config and run virtual functions are the same
  };
  
  /* UpcastTask
   * ----------
   * Provides a mechanism to ensure Task types that are only used with the appropriate Agent types.
   */
  template<typename TAgent>
  class UpcastTask: public fetch::Task
  { public:
              unsigned int config (Agent *agent)              {return config(dynamic_cast<TAgent*>(agent));}
              unsigned int run    (Agent *agent)              {return run   (dynamic_cast<TAgent*>(agent));}
      virtual unsigned int config (TAgent *agent) = 0;
      virtual unsigned int run    (TAgent *agent) = 0;
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
    virtual unsigned int update(Agent *d) = 0; //return 1 on success, 0 otherwise
  };
  
  template<typename TAgent>
  class IUpdateableCast: public IUpdateable
  { public:
            unsigned int update(Agent *d)  {return update(dynamic_cast<TAgent*>(d));}
    virtual unsigned int update(TAgent *d) = 0;
  };

}
