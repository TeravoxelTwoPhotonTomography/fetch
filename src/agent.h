/*
 * Agent.h
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
#include "asynq.h"
//#include "task.h"

//
// Class Agent
// -----------
//
// This object is thread safe and requires no explicit locking.
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
//      Agent()        Attach        Arm        Run
//  NULL <==> Instanced <==> Holding <==> Armed <==> Running
//      ~Agent()       Detach       Disarm      Stop
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
// being deallocated.  The destructor will attempt to properly shutdown from
// any state, but it might not do so gracefully.
//
// The "Back" transition methods (~Agent(), Detach, Disarm, and Stop) accept
// any upstream state.  That is, Detach() will attempt to Stop() and then
// Disarm() a running Agent.  Additionally, Disarm() can be called from any
// state.
//
// OTHER METHODS
// =============
//
// static method
// <connect>
//      Destination channel inherits the existing channel's properties.
//      If both channels exist, the source properties are inherited.
//      One channel must exist.
//
// ABSTRACTION RULES
// =================
// 0. Children must maintain the behaviors outlined above.
//
// 1. Children must impliment:
//
//      unsigned int attach()
//      unsigned int detach()
//
//    <detach> Should attempt to disarm, if possible, and warn when disarm() fails.
//             Should return 0 on success, non-zero otherwise.
//             Should explicitly lock()/unlock() when necessary.
//
//    <attach> Should return 0 on success, non-zero otherwise.
//             Should explicitly lock()/unlock() when necessary.
//
// 2. Children must allocate the required input and output channels on 
//    instatiation.  Preferably with the provided Agent::_alloc_qs functions.
//
//    The buffer sizes for the queue may be approximate.  The important thing
//    is that the device specify the number of queues and the number of buffers
//    on each queue.
//
// 3. Children must impliment destructors that call detach() and handle any errors.
//

#define AGENT_DEFAULT_TIMEOUT INFINITE

namespace fetch {

  typedef asynq* PASYNQ;  
  
  TYPE_VECTOR_DECLARE(PASYNQ);

  class Task;
  
  class Agent
  { 
    public:
               Agent(void);
      virtual ~Agent(void);

      // State transition functions
      virtual unsigned int attach (void) = 0;                  // Returns 0 on success, nonzero otherwise.
      virtual unsigned int detach (void) = 0;                  // Returns 0 on success, nonzero otherwise.  Should attempt to disarm if running.  Should not panic if possible.      

              unsigned int arm    (Task *t, DWORD timeout_ms); // Returns 0 on success, nonzero otherwise.
      virtual unsigned int disarm (DWORD timeout_ms);          // Returns 0 on success, nonzero otherwise.
              unsigned int run    (void);                      // Returns 0 on success, nonzero otherwise.
              unsigned int stop   (DWORD timeout_ms);          // Returns 0 on success, nonzero otherwise.
      
      BOOL      attach_nonblocking (void);
      BOOL      detach_nonblocking (void);
      BOOL         arm_nonblocking (Task *t, DWORD timeout_ms);
      BOOL      disarm_nonblocking (DWORD timeout_ms);
      BOOL         run_nonblocking (void);
      BOOL        stop_nonblocking (DWORD timeout_ms);

      // State query functions      
      unsigned int is_attached(void);  // True in all but the instanced state.
      unsigned int is_available(void); // If 1, Agent is armable.  This is usually only when the Agent is in the "Holding" state.
      unsigned int is_armed(void);
      unsigned int is_runnable(void);
      unsigned int is_running(void);
      unsigned int is_stopping(void);  // use this for testing for main-loop termination in Tasks.
                                       // FIXME: not clear from name that is_stopping is very different from is_running
                                       
      unsigned int wait_till_stopped(DWORD timeout_ms);                                       
                                             
      // Queue manipulation
      static void connect(Agent *dst, size_t dst_chan, Agent *src, size_t src_chan);
      
    public:
      Task            *task;

      vector_PASYNQ   *in,         // Input  pipes
                      *out;        // Output pipes

    public:
      // _alloc_qs
      // _alloc_qs_easy
      //   Allocate <n> independant asynchronous queues.  These are contained
      //   in the vector, *<qs> which is also allocated.  The "easy" version
      //   constructs all <n> queues with <nbuf> buffers, each buffer with the
      //   same initial size, <nbytes>.
      //
      //   These free existing queues before allocing.
      //
      // _free_qs
      //   Safe for calling with *<qs>==NULL. <qs> must not be NULL.
      //   Sets *qs to NULL after releasing queues.
      // 
      static void _alloc_qs      (vector_PASYNQ **qs, size_t n, size_t *nbuf, size_t *nbytes);
      static void _alloc_qs_easy (vector_PASYNQ **qs, size_t n, size_t nbuf, size_t nbytes);
      static void _free_qs       (vector_PASYNQ **qs);
      
    protected:
      friend class Task;
      
      void   lock(void);
      void unlock(void);

      void set_available(void);

    public: // Section (Data): treat as protected, friended to children of Task
      
      HANDLE           thread,
                       notify_available,
                       notify_stop;
      CRITICAL_SECTION _lock;
      u32              num_waiting,
                       _is_available,
                       _is_running;
                       
    private:
      inline static unsigned _handle_wait_for_result     (DWORD result, const char *msg);
      inline        unsigned _wait_for_available         (DWORD timeout_ms);
                    Agent*   _request_available_unlocked (int is_try, DWORD timeout_ms);
  };
}
