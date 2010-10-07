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

#include "asynq.h"
#include "object.h"

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
// RULES
// =====
// 0. Children must maintain the behaviors outlined above.
//
// 1. Children must implement:
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
// TODO
// ====
// - Rename Agent to something like Context or RunLevel or something
// - remove operations on queues to IDevice.  Tasks should be responsible for 
//   flushing queues on shutdown and that way the number of queues can be device
//   dependent and supported by the particular device implementation.
// - update documentation
//

/*
IDevice
=======

Interface class defining two methods:

  - attach()
  - detach()

These functions are called by the Agent class during 
transitions to/from different runstates.

IConfigurableDevice
===================

Inherits two interfaces

  - IDevice
  - Configurable

IDevice is described above.  Configurable is an templated
interface class providing a single method:

  - set_config()

One shouldn't really think of Configurable as doing much;
it's meant to allow a uniform way of accessing a configuration
variable that could be any type.

IConfigurable device provides some other methods.  The two
of interest are:

  - set_config_nowait(Config*)
  - _set_config__locked(Config*)

_set_config__locked() provides a hook for subclasses to propigate
configuration changes to sub-devices.  This function is always
called when devices are stopped and is also called inside of 
a locked mutex.

set_config_nowait() is used to commit configuration changes to
devices without in a non-blocking fashion.. This is accomplished
by posting requests to a queue.  Threads from a thread pool 
execute the requests asynchronously.  Since requests might be 
posted out of order, a timestamp is used to track the request
order; only the most recent requests are executed.  This means
that some requests may be ignored.
*/

#define AGENT_DEFAULT_TIMEOUT INFINITE

namespace fetch {

  typedef asynq* PASYNQ;  
  
  TYPE_VECTOR_DECLARE(PASYNQ);

  class Task;
  class Agent;

  //
  // IDevice
  //
  class IDevice
  {
  public:

    IDevice(Agent* agent);
    virtual ~IDevice();

    virtual unsigned int attach(void)=0;// Returns 0 on success, nonzero otherwise.
    virtual unsigned int detach(void)=0;// Returns 0 on success, nonzero otherwise.  Should attempt to disarm if running.  Should not panic if possible.
  
    // Queue manipulation
    static void connect(IDevice *dst, size_t dst_chan, IDevice *src, size_t src_chan);

    Agent* _agent;

    vector_PASYNQ   *_in,         // Input  pipes
                    *_out;        // Output pipes
  public:
    // _alloc_qs
    // _alloc_qs_easy
    //   Allocate <n> independent asynchronous queues.  These are contained
    //   in the vector, *<qs> which is also allocated.  The "easy" version
    //   constructs all <n> queues with <nbuf> buffers, each buffer with the
    //   same initial size, <nbytes>.
    //
    //   These free existing queues before alloc'ing.
    //
    // _free_qs
    //   Safe for calling with *<qs>==NULL. <qs> must not be NULL.
    //   Sets *qs to NULL after releasing queues.
    // 
    static void _alloc_qs      (vector_PASYNQ **qs, size_t n, size_t *nbuf, size_t *nbytes);
    static void _alloc_qs_easy (vector_PASYNQ **qs, size_t n, size_t nbuf, size_t nbytes);
    static void _free_qs       (vector_PASYNQ **qs);

  private:
    IDevice() {};
  };  
  
  //
  // IConfigurableDevice
  //    declaration
  //
  // Notes
  // - children should overload _assign_config to propagate configuration assignment
  //
  //
  template<class Tcfg>
  class IConfigurableDevice : public IDevice, public Configurable<Tcfg>
  {
  public:
    IConfigurableDevice(Agent *agent);
    IConfigurableDevice(Agent *agent, Config *config);
    virtual void get_config       (Config OUT *cfg);
    virtual void set_config       (Config  IN *cfg);    
            void set_config_nowait(Config *cfg);

  protected:
    virtual void _assign_config(Config IN *cfg) {_config=cfg};
    virtual void _copy_config(Config OUT *cfg) {*cfg=_config};

  private:
    inline void transaction_lock();
    inline void transaction_unlock();
    void _set_config__locked( Config  IN *cfg );

    static DWORD WINAPI _set_config_nowait__helper(LPVOID lparam);

    CRITICAL_SECTION _transaction_lock;
  };

  //
  // Agent
  //
  class Agent
  { 
    public:
               Agent(IDevice *owner);
      virtual ~Agent();

      // State transition functions
      unsigned int attach (void);                      // Returns 0 on success, nonzero otherwise.
      unsigned int detach (void);                      // Returns 0 on success, nonzero otherwise.  Should attempt to disarm if running.  Should not panic if possible.

      unsigned int arm    (Task *t, DWORD timeout_ms); // Returns 0 on success, nonzero otherwise.
      unsigned int disarm (DWORD timeout_ms);          // Returns 0 on success, nonzero otherwise.
      unsigned int run    (void);                      // Returns 0 on success, nonzero otherwise.
      unsigned int stop   (DWORD timeout_ms);          // Returns 0 on success, nonzero otherwise.
      
      BOOL      attach_nowait (void);
      BOOL      detach_nowait (void);
      BOOL         arm_nowait (Task *t, DWORD timeout_ms);
      BOOL      disarm_nowait (DWORD timeout_ms);
      BOOL         run_nowait (void);
      BOOL        stop_nowait (DWORD timeout_ms);

      // State query functions      
      unsigned int is_attached(void);  // True in all but the instanced state.
      unsigned int is_available(void); // If 1, Agent is arm-able.  This is usually only when the Agent is in the "Holding" state.
      unsigned int is_armed(void);
      unsigned int is_runnable(void);
      unsigned int is_running(void);
      unsigned int is_stopping(void);  // use this for testing for main-loop termination in Tasks.
                                       // FIXME: not clear from name that is_stopping is very different from is_running
                                       
      unsigned int wait_till_stopped(DWORD timeout_ms);
      
    public:
      IDevice         *_owner;
      Task            *_task;
      
    protected:
      friend class Task;
      
      void   lock(void);
      void unlock(void);

      void set_available(void);

    public: // Section (Data): treat as protected, friended to children of Task
      
      HANDLE           _thread,
                       _notify_available,
                       _notify_stop;
      CRITICAL_SECTION _lock;
      u32              _num_waiting,
                       _is_available,
                       _is_running;
                       
    private:
      inline static unsigned _handle_wait_for_result     (DWORD result, const char *msg);
      inline        unsigned _wait_for_available         (DWORD timeout_ms);
                    Agent*   _request_available_unlocked (int is_try, DWORD timeout_ms);
  };

////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////

  //
  // IConfigurableDevice
  //    implementation
  //

  template<class Tcfg>
  IConfigurableDevice<Tcfg>::IConfigurableDevice(Agent *agent)
    :IDevice(agent)
    ,Configurable()
  {
    Guarded_Assert_WinErr(InitializeCriticalSectionAndSpinCount(&_transaction_lock,0x80000400));
  }

  template<class Tcfg>
  IConfigurableDevice<Tcfg>::IConfigurableDevice( Agent *agent, Config *config )
    :IDevice(agent)
    ,Configurable(config)
  {
    Guarded_Assert_WinErr(InitializeCriticalSectionAndSpinCount(&_transaction_lock,0x80000400));
  }

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::get_config( Config *cfg )
  {
    transaction_lock();
    _copy_config(cfg);
    transaction_unlock();
  }

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::set_config( Config *cfg )
  {    
    transaction_lock();
    _set_config__locked(cfg);
    transaction_unlock();
  }

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::set_config_nowait( Config *cfg )
  { 
    typedef IConfigurableDevice<Tcfg> TSelf;
    struct T {TSelf *self;Config *cfg;int time;};
    struct T v = {this,cfg,0};
    static asynq *q=NULL;
    static int timestamp=0;

    v.time = ++timestamp;
    if( !q )
      q = Asynq_Alloc(2, sizeof(struct T) );
    if( !Asynq_Push_Copy(q, &v, sizeof(T), TRUE /*expand queue when full*/) )
    { 
      warning("In set_config_nowait(): Could not push request arguments to queue.");
      return 0;
    }
    return QueueUserWorkItem(&TSelf::_set_config_nowait__helper, (void*)q, NULL /*default flags*/);
  }

  template<class Tcfg>
  DWORD WINAPI IConfigurableDevice<Tcfg>::_set_config_nowait__helper( LPVOID lparam )
  {
    typedef IConfigurableDevice<Tcfg> TSelf;
    struct T {TSelf *self;Config *cfg;int time;};
    struct T v = {NULL, 0.0, 0};
    asynq *q = (asynq*) lparam;
    static int lasttime=0;

    if(!Asynq_Pop_Copy_Try(q,&v,sizeof(T)))
    { 
      warning("In set_config_nowait helper procedure:\r\n"
        "\tCould not pop arguments from queue.\r\n");
      return 0;
    }
    debug( "De-queued request:  Config(%p): %f V\t Timestamp: %d\tQ capacity: %d\r\n",v.self, v.cfg, v.time, q->q->ring->nelem );
    Guarded_Assert(v.self);
    v.self->transaction_lock();
    if( (v.time - lasttime) > 0 )  // The <time> is used to synchronize "simultaneous" requests
    { 
      lasttime = time;             // Only process requests dated after the last request.
      v.self->_set_config__locked(v.cfg);
    }
    v.self->transaction_unlock();
    return 0; // success
  }

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::_set_config__locked( Config * cfg )
  {
    _assign_config(cfg);
    if(_agent->is_armed())
    {
      int run = _agent->is_running();
      if(run)
        _agent->stop(AGENT_DEFAULT_TIMEOUT);
      dynamic_cast<fetch::IUpdateable*>(this->_task)->update(this); //commit
      if(run)
        _agent->run();
    }
  }

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::transaction_unlock()
  {EnterCriticalSection(&_transaction_lock);}

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::transaction_lock()
  {LeaveCriticalSection(&_transaction_lock);}

  // end namespaces
  // fetch
}