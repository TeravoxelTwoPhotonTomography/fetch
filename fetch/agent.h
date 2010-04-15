#pragma once

#include "stdafx.h"

//
// Class Agent
// -----------
//
// This models a stateful asynchronous producer and/or consumer associated with
// some resource.  An Agent is a collection of:
//
//      1. an abstract context.  For example, a handle to a hardware resource.
//      2. a dedicated thread
//      3. a Task (see class Task)
//      4. communication channels (see asynq)
//
// Switching between tasks is performed by transitioning through a series of
// states, as follows:
//
//      Alloc          Attach        Arm        Run
//  NULL <==> Instanced <==> Holding <==> Armed <==> Running
//       Free          Detach       Disarm      Stop
//
//       States      Context  Task Available Running flag set 
//       ------      -------  ---- --------- ---------------- 
//       Instanced   No       No     No            No         
//       Holding     Yes      No     Yes           No         
//       Armed       Yes      Yes    Yes           No         
//       Running     Yes      Yes    No            Yes        
//
// Agents must undergo a two-stage initialization.  After an agent object is
// initially constructed, a context must be assigned via the Attach() method.
// Once a context is assigned, the Agent is "available" meaning it may be
// assigned a task via the Arm() method.  Once armed, the Run() method can be
// used to start the assigned task.
//
// As indicated by the model, an Agent should be disarmed and detached before
// being deallocated.
//
// This object is thread safe and requires no explicit locking.
//
namespace fetch {
  typedef void Task;

  typedef asynq* PASYNQ;
  TYPE_VECTOR_DECLARE(PASYNQ);

  class Agent
  { 
    public:
      Agent(void);
      ~Agent(void);

      // State transition functions
      virtual unsigned int attach (void) = 0;
      virtual unsigned int detach (void) = 0;

      unsigned int arm    (Task *t, DWORD timeout_ms);
      unsigned int disarm (DWORD timeout_ms);
      unsigned int run    (void);
      unsigned int stop   (DWORD timeout_ms);
      
      BOOL         arm_nonblocking(Task *t, DWORD timeout_ms);

      // State query functions
      unsigned int is_available(void);
      unsigned int is_armed(void);
      unsigned int is_runnable(void);
      unsigned int is_running(void);

    protected:
      void   lock(void);
      void unlock(void);

      void set_available(void);

      vector_PASYNQ *in,         // Input  pipes
                    *out;        // Output pipes

    private:
      friend class Task;

      HANDLE           thread,
                       notify_available,
                       notify_stop;
      CRITICAL_SECTION lock;
      u32              num_waiting,
                       _is_available,
                       _is_running;
      Task            *task;
      void            *context;
  }
}
