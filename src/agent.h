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

#include "common.h"
#include "chan.h"
#include "object.h"
#include "util\util-protobuf.h"

/** \class fetch::Agent
    \brief Models the run state associated with some resource.

    This object is thread safe and requires no explicit locking by callers.

    An Agent can be associated with one or more \ref IDevice objects.  These
    go through state changes together.  One special \ref IDevice is the agent's
    "owner."  This device gets notified of state changes via a callback.
    This system of callbacks is most of what defines the \ref IDevice interface.
    For example, when the agent tries to attach(), it calls the owner's on_device()
    method.

    The callbacks return a value indicating whether the transition was successful.

    The run state is modeled as follows:

\verbatim
         Agent()        Attach        Arm        Run
     NULL <==> Instanced <==> Holding <==> Armed <==> Running
         ~Agent()       Detach       Disarm      Stop
\endverbatim

    The arm() step associates the \ref Agent with a \ref fetch::Task.  Only one
    \ref fetch::Task can be associated at a time.  It defines a function that
    will be run in it's own thread when the \ref Agent is run().  When that
    function returns, the \ref Agent will go back to the armed() state.

    After an agent object is initially constructed, a context must be assigned
    via the Attach() method.  Once a context is assigned, the Agent is "available"
    meaning it may be assigned a task via the Arm() method.  Once armed, the
    Run() method can be used to start the assigned task.

\verbatim
          States      Context  Task Available Running flag set
          ------      -------  ---- --------- ----------------
          Instanced   No       No     No            No
          Holding     Yes      No     Yes           No
          Armed       Yes      Yes    Yes           No
          Running     Yes      Yes    No            Yes
\endverbatim

    As indicated by the model, an Agent should be disarmed and detached before
    being deallocated.  The destructor will attempt to properly shutdown from
    any state, but it might not do so gracefully.

    The "Back" transition methods (~Agent(), Detach, Disarm, and Stop) accept
    any upstream state.  That is, Detach() will attempt to Stop() and then
    Disarm() a running Agent.  Additionally, Disarm() can be called from any
    state.

    \todo A better design would be to use a listener pattern.
          Refactor \ref IDevice so it's explicit that it's just the listener
          interface.
    \todo Rename Agent to something like Context or RunLevel or something
    \todo remove operations on queues to IDevice.  Tasks should be responsible for
          flushing queues on shutdown and that way the number of queues can be device
          dependent and supported by the particular device implementation.
    \todo update documentation
 */

#define AGENT_DEFAULT_TIMEOUT INFINITE

namespace fetch {

  typedef Chan* PCHAN;

  TYPE_VECTOR_DECLARE(PCHAN);

  class Task;
  class Agent;

  //  IDevice
  /** Interface class defining callbacks to handle \ref Agent state changes.

      The interface is usually implemented by device classes, hence the name.

      There's also some utilities for using \ref Chan's.

      Don't call these directly.  They're called through Agent::attach()/detach().
      The agent class takes care of synchronization and non-blocking calls.

      \todo These should probably moved to another class at some point.
      \todo Refactor IDevice to something like IRunStateListener.
   */

  class IDevice
  {
  public:

    IDevice(Agent* agent);
    virtual ~IDevice();

    virtual unsigned int on_attach(void)=0;///< Returns 0 on success, nonzero otherwise.
    virtual unsigned int on_detach(void)=0;///< Returns 0 on success, nonzero otherwise.  Should attempt to disarm if running.  Should not panic if possible.

    virtual unsigned int on_disarm(void) {return 0;/*success*/} ///< Returns 0 on success, nonzero otherwise

    // Queue manipulation
    static void connect(IDevice *dst, size_t dst_chan, IDevice *src, size_t src_chan);

    /// Called when device is already stopped, just after changes are commited via _set_config (See IConfigurableDevice)
    /// Overload this to commit state changes that require more than a stop/run cycle.
    /// Can also be used to as a (potentially blocking) callback that is called just after changes are commited.
    virtual void onUpdate() {}


    Agent* _agent;

    vector_PCHAN   *_in,         ///< Input  pipes
                   *_out;        ///< Output pipes

  public:
    /// \fn _alloc_qs
    ///     Allocate n independent asynchronous queues.  These are contained
    ///     in the vector, *qs which is also allocated.
    ///     These free existing queues before alloc'ing.
    /// \fn _alloc_qs_easy
    ///     constructs all n queues with nbuf buffers, each buffer with the
    ///     same initial size, nbytes
    ///
    /// \fn _free_qs
    ///     Safe for calling with *qs==NULL. qs must not be NULL.
    ///     Sets *qs to NULL after releasing queues.
    ///
    static void _alloc_qs      (vector_PCHAN **qs, size_t n, size_t *nbuf, size_t *nbytes);
    static void _alloc_qs_easy (vector_PCHAN **qs, size_t n, size_t nbuf, size_t nbytes);
    static void _free_qs       (vector_PCHAN **qs);

  private:
    IDevice() {};
  };

  //
  // IConfigurableDevice
  //
  /// - children should overload _set_config to propagate configuration assignment
  /// - children should overload _get_config to change how the referenced configuration
  ///   is modified on assignment.
  ///
  /// - "get" "set" and "set_nowait" make sure that configuration changes get committed
  ///   to the device in a sort-of transactional manner; they take care of changing
  ///   the run state of the device and synchronizing changes.  In the end they call
  ///   the "_set" or "_get" functions to get the job done.
  ///
  /// - "update" can be useful when classes want to re-implement the config "get" and
  ///   "set" functions.  "update" performs the changes required for armed devices.
  ///   Armed devices have had a Task's config() function run.  The Task::config() function
  ///   is sometimes overkill for small parameter changes. onUpdate() and Task::update()
  ///   are provided for just this reason.

  template<class Tcfg>
  class IConfigurableDevice : public IDevice, public Configurable<Tcfg>
  {
  public:
    IConfigurableDevice(Agent *agent);
    IConfigurableDevice(Agent *agent, Config *config);
    virtual Config get_config(void);            ///< see: _get_config()        - returns a snapshot of the config
    virtual void set_config(Config *cfg);       ///< see: _set_config(Config*) - assigns the address of the config
    virtual void set_config(const Config &cfg); ///< see: _set_config(Config&) - update via copy
    int set_config_nowait(const Config& cfg);   ///< see: _set_config(Config&) - update via copy

  public:
    // Overload these.
    // Each are called within a "transaction lock"
    // Use this version inside of constructors rather than set_config().  These guys shouldn't require a constructed agent.
    virtual void _set_config(Config IN *cfg)      {_config=cfg;}     ///< changes the pointer
    virtual void _set_config(const Config &cfg)   {*_config=cfg;}    ///< copy
    virtual void _get_config(Config **cfg)        {*cfg=_config;}    ///< get the pointer
    virtual const Config& _get_config()           {return *_config;} ///< get a const reference (a snapshot to copy)

    inline void transaction_lock();
    inline void transaction_unlock();
  protected:
    virtual void update();               ///< This stops a running agent, calls the onUpdate() function, restarting the agent as necessary.


  private:
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
               Agent(char* name, IDevice *owner);

               void __common_setup();

               virtual ~Agent();

      // State transition functions
      unsigned int attach (void);                      ///< Returns 0 on success, nonzero otherwise.
      unsigned int detach (void);                      ///< Returns 0 on success, nonzero otherwise.  Should attempt to disarm if running.  Should not panic if possible.

      unsigned int arm    (Task *t, IDevice *dc, DWORD timeout_ms=INFINITE); ///< Returns 0 on success, nonzero otherwise.
      unsigned int disarm (DWORD timeout_ms=INFINITE); ///< Returns 0 on success, nonzero otherwise.
      unsigned int run    (void);                      ///< Returns 0 on success, nonzero otherwise.
      unsigned int stop   (DWORD timeout_ms=INFINITE); ///< Returns 0 on success, nonzero otherwise.

      BOOL      attach_nowait (void);
      BOOL      detach_nowait (void);
      BOOL         arm_nowait (Task *t, IDevice *dc, DWORD timeout_ms=INFINITE);
      BOOL      disarm_nowait (DWORD timeout_ms=INFINITE);
      BOOL         run_nowait (void);
      BOOL        stop_nowait (DWORD timeout_ms=INFINITE);

      // State query functions
      unsigned int is_attached(void);  ///< True in all but the instanced state.
      unsigned int is_available(void); ///< If 1, Agent is arm-able.  This is usually only when the Agent is in the "Holding" state.
      unsigned int is_armed(void);
      unsigned int is_runnable(void);
      unsigned int is_running(void);
      unsigned int is_stopping(void);  ///< use this for testing for main-loop termination in Tasks.
                                       /// \todo FIXME: not clear from name that is_stopping is very different from is_running

      unsigned int wait_till_stopped(DWORD timeout_ms=INFINITE);   ///< returns 1 on success, 0 otherwise
      unsigned int wait_till_available(DWORD timeout_ms=INFINITE); ///< returns 1 on success, 0 otherwise

      unsigned int last_run_result(); ///< returns the value returned by the run function from the most recently stopped task.

      char *name() {return _name?_name:" \0";}

    public:
      IDevice         *_owner;
      Task            *_task;

      char *_name;

    protected:

      void set_available(void);  ///<indicates agent is ready for arming


    public: // treat these like they're private
      void   lock(void);
      void unlock(void);

    public: // Section (Data): treat as protected, friended to children of Task

      HANDLE           _thread,
                       _notify_available,
                       _notify_stop;
      CRITICAL_SECTION _lock;
      u32              _num_waiting,
                       _is_available,
                       _is_running;
      unsigned         _last_run_result;

    private:
      inline static unsigned _handle_wait_for_result     (DWORD result, const char *msg);
                    Agent*   _request_available_unlocked (int is_try, DWORD timeout_ms);
      inline unsigned int    _wait_till_available(DWORD timeout_ms);///< private because it requires a particular locking pattern
  };

  //end namespace fetch
}

////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////

#include "task.h"

namespace fetch {
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

  //************************************
  // Method:    get_config
  // FullName:  fetch::IConfigurableDevice<Tcfg>::get_config
  // Access:    virtual public
  // Returns:   Config
  // Qualifier:
  // Parameter: void
  //
  /// Returns a copy of _config.  Synchronized.
  /// Overload _get_config() to change how copy is performed.
  //************************************
  template<class Tcfg>
  Tcfg IConfigurableDevice<Tcfg>::get_config(void)
  {
    transaction_lock();
    Tcfg cfg(*_config);
    transaction_unlock();
    return cfg;
  }

  //************************************
  // Method:    set_config
  // FullName:  fetch::IConfigurableDevice<Tcfg>::set_config
  // Access:    virtual public
  // Returns:   void
  // Qualifier:
  // Parameter: Config * cfg
  //
  /// Synchronized setting of the config.
  /// Overload _set_config to change how copy is performed.
  /// May wait on hardware.
  //
  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::set_config(Tcfg *cfg)
  {
    int run;
    _agent->lock(); //will generate a recursive lock :(
    if( *cfg != _get_config() ) // make sure an commit is required - requires != defined for all Tcfg
    { run = _agent->is_running();
      if(run)
        _agent->stop(AGENT_DEFAULT_TIMEOUT);

      transaction_lock();
      _set_config(cfg);
      transaction_unlock();
      update();

      if(run)
        _agent->run();
    }
    _agent->unlock();
  }

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::set_config( const Config& cfg )
  {
    int run;
    _agent->lock();
    if( cfg != _get_config() ) // make sure an commit is required - requires != defined for all Tcfg
    { run = _agent->is_running();
      if(run)
        _agent->stop(AGENT_DEFAULT_TIMEOUT); //will generate a recursive lock :(

      transaction_lock();
      _set_config(cfg);
      transaction_unlock();
      update();

      if(run)
        _agent->run();
    }
    _agent->unlock();
  }

#define CONFIG_BUFFER_MAX_BYTES 2048
  //************************************
  // Method:    set_config_nowait
  // FullName:  fetch::IConfigurableDevice<Tcfg>::set_config_nowait
  // Access:    public
  // Returns:   void
  // Qualifier: A copy to the object pointed to by <cfg> is posted
  //            to a queue, so <cfg> does not need to live until
  //            the request is committed.
  // Parameter: const Config & cfg
  ///           Performs an extra copy on Push/Pop.  I think it's
  ///           3-4 copies per request.  I could get this down to
  ///           1-2 using a non-copy Push/Pop, I think.  However,
  ///           performance should not be an issue here.
  //************************************
  template<class Tcfg>
  int IConfigurableDevice<Tcfg>::set_config_nowait( const Config &cfg )
  {
    typedef IConfigurableDevice<Tcfg> TSelf;
    struct T {TSelf *self;u8 data[CONFIG_BUFFER_MAX_BYTES]; size_t size; int time;};
    struct T v = {0};
    static Chan *writer, *q=NULL;
    static int timestamp=0;
    //memset(&v,0,sizeof(v));
    v.self = this;
    v.time = ++timestamp;


    v.size = cfg.ByteSize();
    Guarded_Assert(cfg.SerializeToArray(v.data,CONFIG_BUFFER_MAX_BYTES));


    if( !q )
    { q = Chan_Alloc(2, sizeof(T) );
      Chan_Set_Expand_On_Full(q,1);
    }
    writer=Chan_Open(q,CHAN_WRITE);
    if(!CHAN_SUCCESS( Chan_Next_Copy(writer, &v, sizeof(T)) ))
    {
      warning("In set_config_nowait(): Could not push request arguments to queue.");
      return 0;
    }
    Chan_Close(writer);

    BOOL sts;
    sts = QueueUserWorkItem(&TSelf::_set_config_nowait__helper, (void*)q, NULL /*default flags*/);

    return sts;
  }

  template<class Tcfg>
  DWORD WINAPI IConfigurableDevice<Tcfg>::_set_config_nowait__helper( LPVOID lparam )
  {
    DWORD err = 0; //success
    typedef IConfigurableDevice<Tcfg> TSelf;
    struct T {TSelf *self;u8 data[CONFIG_BUFFER_MAX_BYTES]; size_t size; int time;};
    struct T v = {0};
    Chan *reader,*q = (Chan*) lparam;
    static int lasttime=0;
    //bool updated = false;

    //memset(&v,0,sizeof(v));

    reader = Chan_Open(q,CHAN_READ);
    if(!CHAN_SUCCESS( Chan_Next_Copy(reader,&v,sizeof(T)) ))
    {
      warning(
        "In set_config_nowait helper procedure:\r\n"
        "\tCould not pop arguments from queue.\r\n");
      return 0;
    }
    //debug( "De-queued request:  Config(0x%p): %d V\t Timestamp: %d\tQ capacity: %d\r\n",v.data, v.size, v.time, q->q->ring->nelem );
    Guarded_Assert(v.self);
    if( (v.time - lasttime) > 0 )  // The <time> is used to synchronize "simultaneous" requests
    { Config cfg(v.self->get_config());
      lasttime = v.time;             // Only process requests dated after the last request.
      goto_if_fail(cfg.ParseFromArray(v.data,(int)v.size),FailedToParse);
      v.self->set_config(cfg);
      //updated = true;
    }
Finalize:
    Chan_Close(reader);
    //if(updated)          // Fix?: update appears to be called within set_config.
    //  v.self->update();
    return err;
FailedToParse:
    pb::unlock();
    warning("Failed to update the config.\r\n");
    err = 1;
    goto Finalize;
  }

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::transaction_unlock()
  {LeaveCriticalSection(&_transaction_lock);}

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::transaction_lock()
  {EnterCriticalSection(&_transaction_lock);}

  template<class Tcfg>
  void IConfigurableDevice<Tcfg>::update()
  {
    if(_agent->is_armed())
    {
      int run;
      _agent->lock(); //will generate a recursive lock :(
      run = _agent->is_running();
      if(run)
        _agent->stop(AGENT_DEFAULT_TIMEOUT);
      _agent->_owner->onUpdate();
      if(run)
      { //HERE;
        _agent->run();
      }
      _agent->unlock();
    }

  }

// end namespaces
// fetch
}
