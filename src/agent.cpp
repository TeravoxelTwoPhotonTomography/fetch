#include "common.h"
#include "agent.h"
#include "task.h"

#define DEBUG_AGENT__HANDLE_WAIT_FOR_RESULT

#if 0
#define DBG(...) debug(__VA_ARGS__)
#define LOCKDBG(...)  //DBG(__VA_ARGS__)
#else
#define DBG(...)
#define LOCKDBG(...)
#endif

#if 1
#define DBG_RUN      DBG("Run:    %20s 0x%p"ENDL                               \
                         "        [thread %04d] (sts %d)"ENDL                  \
                         "        in 0x%p  out 0x%p"ENDL,                      \
                        name(),this,GetThreadId(_thread), sts,                 \
                        (_owner->_in )?Chan_Id(_owner->_in->contents[0]):NULL, \
                        (_owner->_out)?Chan_Id(_owner->_out->contents[0]):NULL)
#define DBG_STOP     DBG("Stop     %20s 0x%p"ENDL,name(), this)
#define DBG_ARMED    DBG("Armed    %20s 0x%p"ENDL,name(), this)
#define DBG_DISARMED DBG("Disarmed %20s 0x%p"ENDL,name(), this)
#define DBG_ATTACHED DBG("Attached %20s 0x%p"ENDL,name(), this)
#define DBG_DETACHED DBG("Detached %20s 0x%p"ENDL,name(), this)
#else
#define DBG_ARMED
#endif

namespace fetch
{
    TYPE_VECTOR_DEFINE( PCHAN );

  inline void
    Agent::lock(void)
  {
      LOCKDBG("[%s] (-) LockCount: %d\r\n",name(),_lock.LockCount);
      EnterCriticalSection(&_lock);
      LOCKDBG("[%s] (+) LockCount: %d\r\n",name(),_lock.LockCount);
    }

  inline void
    Agent::unlock(void)
    {
      LOCKDBG("[%s] (+) Un LockCount: %d\r\n",name(), _lock.LockCount);
      LeaveCriticalSection(&_lock);
      LOCKDBG("[%s] (-) Un LockCount: %d\r\n",name(), _lock.LockCount);
    }

    Agent::Agent(IDevice *owner) :
      _thread(INVALID_HANDLE_VALUE),
      _is_available(0),
      _is_running(0),
      _owner(owner),
      _task(NULL),
      _num_waiting(0),
      _name(0),
      _last_run_result(0)
    {
       __common_setup();
    }

    Agent::Agent(char *name, IDevice *owner) :
      _thread(INVALID_HANDLE_VALUE),
      _is_available(0),
      _is_running(0),
      _owner(owner),
      _task(NULL),
      _num_waiting(0),
      _name(name),
      _last_run_result(0)
    {
      __common_setup();
    }

    inline static void _safe_free_handle(HANDLE *h)
    {
        if(*h != INVALID_HANDLE_VALUE)
            if(!CloseHandle(*h))
                ReportLastWindowsError();
        *h = INVALID_HANDLE_VALUE;
    }

    Agent::~Agent(void)
    {
        //if(this->detach() > 0)                                       // FIXME: This doesn't work
        //    warning("~Agent : Attempt to detach() timed out.\r\n");

        if(_num_waiting > 0)
            warning("[%s] ~Agent : Agent has waiting tasks.\r\n         Try calling Agent::detach first.\r\n.",name());
        _safe_free_handle(&_thread);
        _safe_free_handle(&_notify_available);
        _safe_free_handle(&_notify_stop);
        DeleteCriticalSection(&_lock);
        _task = NULL;
        _owner = NULL;
    }

    /* Returns 1 on success, 0 otherwise.
    * Possibly generates a panic shutdown.
    * Lack of success could indicate the lock was
    *   abandoned or a timeout elapsed.
    * A warning will be generated if the lock was
    *   abandoned.
    * Return of 0 indicates a timeout or an
    *   abandoned wait.
    */
    inline unsigned Agent::_handle_wait_for_result(DWORD result, const char *msg)
    {
        return_val_if( result == WAIT_OBJECT_0, 1 );
        Guarded_Assert_WinErr( result != WAIT_FAILED );
        if(result == WAIT_ABANDONED)
            warning("Agent: Wait abandoned\r\n\t%s\r\n", msg);

        if(result == WAIT_TIMEOUT)
            warning("Agent: Wait timeout\r\n\t%s\r\n", msg);

        return 0;
    }

    inline unsigned Agent::_wait_till_available(DWORD timeout_ms)
    {
        HANDLE notify = this->_notify_available;
        DWORD res;
        Guarded_Assert_WinErr( ResetEvent( notify ) );
        this->_num_waiting++;
        this->unlock();
        res = WaitForSingleObject(notify, timeout_ms);
        this->lock();
        this->_num_waiting--;
        return _handle_wait_for_result(res, "Agent wait for availability.");
    }

    Agent* Agent::_request_available_unlocked(int is_try, DWORD timeout_ms)
    {
      if( !this->_is_available )
      { if( is_try ) return NULL;
        if( !_wait_till_available(timeout_ms))
          if( !this->_is_available )
            return NULL;
      }
      return this;
    }

    inline unsigned int Agent::wait_till_available(DWORD timeout_ms)
    {
      lock();
      Agent *a = _request_available_unlocked(0,timeout_ms);
      unlock();
      return a!=NULL; //returns 1 on success, 0 otherwise
    }
    unsigned int Agent::is_attached(void)
    { return is_available() || this->_task != NULL;
    }

    unsigned int Agent::last_run_result()
    { return _last_run_result;
    }

    unsigned int Agent::is_armed(void)
    {
        return this->_task != NULL;
    }

    unsigned int Agent::is_runnable(void)
    {
        return this->_task != NULL     //task is set
            && !(this->_is_running);
    }

    unsigned int Agent::is_available(void)
    {
        return this->_is_available;
    }

    unsigned int Agent::is_running(void)
    {
        return this->_is_running;
    }

    unsigned int Agent::is_stopping(void)
    { DWORD res = WaitForSingleObject(_notify_stop, 0);
      return WAIT_OBJECT_0 == res;
    }

    void Agent::set_available(void)
    {
        //Notify waiting tasks
        this->_is_available = 1;
        if( this->_num_waiting > 0 )
        Guarded_Assert_WinErr(SetEvent(this->_notify_available));
    }

    unsigned int Agent::wait_till_stopped(DWORD timeout_ms)
    { unsigned int sts = 0;
      if( this->_thread != INVALID_HANDLE_VALUE )
        // thread could still become invalidated between if and wait, but need to enter wait unlocked
        // I expect Wait will return WAIT_FAILED immediately if that's the case, so this isn't really
        // a problem.
        sts = WaitForSingleObject(this->_thread,timeout_ms) == WAIT_OBJECT_0;
      return sts;
    }

#define TRY(expr,lbl) \
    if(!expr) \
    { warning("%s(%d): %s"ENDL"\t%s"ENDL"\tExpression evaluated to false."ENDL,__FILE__,__LINE__,#lbl,#expr); \
      goto lbl; \
    }

    unsigned int Agent::arm(Task *task, IDevice *dc, DWORD timeout_ms)
    { Guarded_Assert(dc->_agent==this);
      lock(); // Source state can not be "armed" or "running"
      TRY(!is_running()&&!is_armed(),Error);
      if(!_request_available_unlocked(0/*is try*/, timeout_ms))// Blocks till agent is available
      {
        warning("[%s] Agent unavailable.  Perhaps another task is running?\r\n\tAborting attempt to arm.\r\n",name());
        goto Error;
      }
      // Exec task config function
      if(!(task->config)(dc)){
        warning("[%s] While loading task, something went wrong with the task configuration.\r\n\tAgent not armed.\r\n",name());
        goto Error;
      }
      // Create thread for running task
      Guarded_Assert_WinErr( this->_thread = CreateThread(
        0,                  // use default access rights
        0,                  // use default stack size (1 MB)
        task->thread_main,  // main function
        dc,                 // arguments
        CREATE_SUSPENDED,   // don't start yet
        NULL )); // don't worry about the thread id
      _task = task; // save the task - this also indicates the agent is armed
      unlock();
      DBG_ARMED;
      return 0;
Error:
      this->_task = NULL;
      this->set_available();
      _owner->on_disarm();
      unlock();
      return 1;
    }

    DWORD WINAPI _agent_arm_thread_proc(LPVOID lparam)
    {
      struct T
      {
        Agent *d;
        IDevice *dc;
        Task *dt;
        DWORD timeout;
      };
      Chan *reader, *q = (Chan*)(lparam);
      T args = {0, 0, 0, 0};
      reader = Chan_Open(q,CHAN_READ);
      if(CHAN_FAILURE( Chan_Next_Copy(reader, &args, sizeof(T)) ))
      {
        warning("In Agent::arm_nonblocking work procedure:"ENDL
                "\tCould not pop arguments from queue."ENDL);
        return 0;
      }
      Chan_Close(reader);
      return args.d->arm(args.dt, args.dc, args.timeout);
    }

    BOOL Agent::arm_nowait(Task *task, IDevice *dc, DWORD timeout_ms)
    {
        struct T
        {
            Agent *d;
            IDevice *dc;
            Task *dt;
            DWORD timeout;
        };
        struct T args = {this, dc, task, timeout_ms};
        static Chan *q = NULL;
        Chan *writer;
        if(!q)
        {
          q = Chan_Alloc(32, sizeof (T));
          Chan_Set_Expand_On_Full(q,1);
        }

        writer = Chan_Open(q, CHAN_WRITE);
        if(CHAN_FAILURE( Chan_Next_Copy(writer,&args,sizeof(T)) )){
            warning("[%s] In Agent::arm_nonblocking: Could not push request arguments to queue.",name());
            return 0;
        }
        Chan_Close(writer);

        BOOL sts;
        sts = QueueUserWorkItem(&_agent_arm_thread_proc, (void*)q, NULL /*default flags*/);
        return sts;
    }

    unsigned int Agent::disarm(DWORD timeout_ms)
    {
      unsigned int sts = 1; //success
      if(this->_is_running)
        // Source state can be running or armed
        this->stop(timeout_ms);

      this->lock();
      this->_task = NULL;
      this->set_available();
      sts &= _owner->on_disarm();
      this->unlock();
      DBG_DISARMED;
      return sts;
    }

    DWORD WINAPI _agent_disarm_thread_proc(LPVOID lparam)
    {
        struct T
        {
            Agent *d;
            DWORD timeout;
        };
        Chan *reader, *q = (Chan*)(lparam);
        struct T args = {0, 0};
        reader = Chan_Open(q,CHAN_READ);
        if(CHAN_FAILURE( Chan_Next_Copy(reader, &args, sizeof(T)) )){
            warning("In Agent::disarm_nonblocking work procedure:\r\n\tCould not pop arguments from queue.\r\n");
            return 0;
        }
        Chan_Close(reader);
        return args.d->disarm(args.timeout);
    }

    BOOL Agent::disarm_nowait(DWORD timeout_ms)
    {
        struct T
        {
            Agent *d;
            DWORD timeout;
        };
        struct T args = {this, timeout_ms};
        static Chan *q = NULL;
        Chan *writer;
        if(!q)
        {   q = Chan_Alloc(32, sizeof (T));
            Chan_Set_Expand_On_Full(q,1);
        }

        writer = Chan_Open(q, CHAN_WRITE);
        if(CHAN_FAILURE( Chan_Next_Copy(writer, &args, sizeof(T)) )){
            warning("[%s] In Agent::disarm_nonblocking:\r\n\tCould not push request arguments to queue.\r\n",name());
            return 0;
        }
        Chan_Close(writer);

        BOOL sts;
        sts = QueueUserWorkItem(&_agent_disarm_thread_proc, (void*)q, NULL /*default flags*/);
        return sts;
    }

    // Transitions from armed to running state
    // Returns 1 on success
    //         0 otherwise
    // A return of 0 indicates a failure to start the thread
    // and could be due to multiple suspensions on the thread.
    unsigned int Agent::run(void)
    {
      DWORD sts = 0;
      this->lock();
      if(this->is_runnable())
      {
        this->_is_running = 1;
        if( this->_thread != INVALID_HANDLE_VALUE )
        {
          sts = ResumeThread(this->_thread) <= 1; // Thread's already allocated so go!
          DBG("Agent Run: ResumeThread id:%d"ENDL,GetThreadId(this->_thread));
        }
        else
        { // Create thread for running the task
          Guarded_Assert_WinErr(
            this->_thread = CreateThread(0, // use default access rights
            0,                  // use default stack size (1 MB)
            this->_task->thread_main, // main function
            this->_owner,       // arguments
            0,                  // run immediately
            NULL ));            // don't worry about the thread id
          sts = 1;
          DBG("Agent Run: Created thread(id:%d) for %s\r\n",GetThreadId(this->_thread),name());
        }
      } else //(then not runnable)
      {
        warning(
          "[%s] Attempted to run an unarmed or already running Agent.\r\n"
          "\tAborting the run attempt.\r\n",name());
      }
      DBG_RUN;

      this->unlock();
      return sts;
    }

    DWORD WINAPI _agent_run_thread_proc(LPVOID lparam)
    {
        struct T
        {
            Agent *d;
        };
        Chan *reader,*q = (Chan*)(lparam);
        struct T args = {0};
        reader=Chan_Open(q,CHAN_READ);
        if(CHAN_FAILURE( Chan_Next_Copy(reader, &args, sizeof(T)) )){
            warning("In Agent::run_nonblocking work procedure:\r\n\tCould not pop arguments from queue.\r\n");
            return 0;
        }
        Chan_Close(reader);
        return args.d->run();
    }

    BOOL Agent::run_nowait()
    {
        struct T
        {
            Agent *d;
        };
        struct T args = {this};
        static Chan *q = NULL;
        Chan *writer;
        return_val_if_fail( this, 0 );
        if(!q)
        {   q = Chan_Alloc(32, sizeof (T));
            Chan_Set_Expand_On_Full(q,1);
        }

        writer=Chan_Open(q,CHAN_WRITE);
        if(CHAN_FAILURE( Chan_Next_Copy(writer, &args, sizeof(T)) )){
            warning("[%s] In Agent_Run_Nonblocking:\r\n\tCould not push request arguments to queue.\r\n",name());
            return 0;
        }
        Chan_Close(writer);

        BOOL sts;
        sts = QueueUserWorkItem(&_agent_run_thread_proc, (void*)q, NULL /*default flags*/);
        return sts;
    }

    // Transitions from running to armed state.
    // Returns 1 on success
    unsigned int Agent::stop(DWORD timeout_ms)
    {
      DWORD res;
      HANDLE t;
      lock();
      DBG("Agent: [ ] Stopping %s 0x%p\r\n",name(), this);
      if( _is_running )
      {
        _is_running = 0;
        if( _thread != INVALID_HANDLE_VALUE)
        { t = _thread;
          unlock();
          res = SignalObjectAndWait(_notify_stop,t,timeout_ms,FALSE);  // notifies and waits on thread to signal
            //res = WaitForSingleObject(t, timeout_ms); // wait for running thread to stop
          lock();
          // Handle a timeout on the wait.
          if( !_handle_wait_for_result(res, "Agent stop: Wait for thread."))
          { Guarded_Assert_WinErr__NoPanic(TerminateThread(_thread,127)); // Force the thread to stop
            warning("%s(%d)"ENDL "\t[%s] Timed out waiting for task thread (%d) to stop."ENDL,__FILE__,__LINE__,name(), GetThreadId(_thread));
          }
          CloseHandle(_thread);
          _thread = INVALID_HANDLE_VALUE;

          ResetEvent(_notify_stop);

          if(_thread!=INVALID_HANDLE_VALUE)
          { CloseHandle(_thread);
            _thread = INVALID_HANDLE_VALUE;
          }
        }
      }
      unlock();
      DBG_STOP;
      return 1;
    }

    DWORD WINAPI _agent_stop_thread_proc(LPVOID lparam)
    {
        struct T
        {
            Agent *d;
            DWORD timeout;
        };
        Chan *reader,*q = (Chan*)(lparam);
        struct T args = {0, 0};
        reader=Chan_Open(q,CHAN_READ);
        if(CHAN_FAILURE( Chan_Next_Copy(reader, &args, sizeof(T)) )){
            warning("%s(%d)"ENDL
                    "\tIn Agent_Stop_Nonblocking work procedure:"ENDL
                    "\tCould not pop arguments from queue."ENDL,__FILE__,__LINE__);
            return 0;
        }
        Chan_Close(reader);
        return args.d->stop(args.timeout);
    }

    BOOL Agent::stop_nowait(DWORD timeout_ms)
    {
        struct T
        {
            Agent *d;
            DWORD timeout;
        };
        struct T args = {this, timeout_ms};
        static Chan *q = NULL;
        Chan * writer;
        if(!q)
        {   q = Chan_Alloc(32, sizeof (T));
            Chan_Set_Expand_On_Full(q,1);
        }

        writer=Chan_Open(q,CHAN_WRITE);
        if(CHAN_FAILURE( Chan_Next_Copy(writer, &args, sizeof(T)) )){
            warning("[%s] In Agent::stop_nonblocking:\r\n\tCould not push request arguments to queue.\r\n",name());
            return 0;
        }
        Chan_Close(writer);

        DBG("Agent: [ ] Non-blocking stop requested for %s 0x%p\r\n",name(), this);
        BOOL sts;
        sts = QueueUserWorkItem(&_agent_stop_thread_proc, (void*)q, NULL /*default flags*/);
        return sts;
    }

    DWORD WINAPI _agent_detach_thread_proc(LPVOID lparam)
    {
        Agent *d = (Agent*)(lparam);
        return d->detach();
    }

    BOOL Agent::detach_nowait()
    {
        return QueueUserWorkItem(&_agent_detach_thread_proc, (void*)this, NULL /*default flags*/);
    }

    DWORD WINAPI _agent_attach_thread_proc(LPVOID lparam)
    {
      Agent *d = (Agent*)(lparam);
      return d->attach();
    }

  BOOL
    Agent::attach_nowait()
    { return QueueUserWorkItem(&_agent_attach_thread_proc, (void*)this, NULL /*default flags*/);
    }

  // Returns 0 on success, nonzero otherwise.
  unsigned int Agent::attach( void )
  { unsigned int sts = 0;
    lock();
    sts |= _owner->on_attach();
    if(sts==0)
      set_available();
    else
      warning("[%s] Agent's owner did not attach properly."ENDL"\tFile: %s (%d)"ENDL,name(),__FILE__,__LINE__);
    unlock();
    DBG_ATTACHED;
    return sts;
  }

  // Returns 0 on success, nonzero otherwise.
  // Should attempt to disarm if running.
  // Should not panic if possible.
  unsigned int Agent::detach( void )
  {
    unsigned int sts=1;
    if(disarm(AGENT_DEFAULT_TIMEOUT))
      warning("[%s] Could not cleanly disarm (device at 0x%p)"ENDL,name(), _owner);
    lock();
    sts = _owner->on_detach();
    _is_available=0;
    unlock();
    DBG_DETACHED;
    return sts;
  }

  void Agent::__common_setup()
  {
    // default security attr
    // manual reset
    // initially unsignaled
    Guarded_Assert_WinErr(
      this->_notify_available = CreateEvent( NULL,  // default security attr
      TRUE,   // manual reset
      FALSE,  // initially unsignalled
      NULL ));
    // default security attr
    // manual reset
    // initially unsignaled
    Guarded_Assert_WinErr(
      this->_notify_stop      = CreateEvent( NULL,  // default security attr
      TRUE,   // manual reset
      FALSE,  // initially unsignalled
      NULL ));
    Guarded_Assert_WinErr(
      InitializeCriticalSectionAndSpinCount( &_lock, 0x8000400 ));
  }



  IDevice::IDevice( Agent* agent )
    :_agent(agent),
    _in(NULL)
    ,_out(NULL)
  {
  }

  IDevice::~IDevice()
  {
    // should detach if attached?

    _free_qs(&_in);
    _free_qs(&_out);
  }

  void IDevice::_free_qs(vector_PCHAN **pqs)
  {
    vector_PCHAN *qs = *pqs;
    if( qs )
    {
      size_t n = qs->count;
      int sts = 1;
      while(n--)
        sts &= Chan_Close( qs->contents[n] );      //qs[n] isn't necessarily deleted. Unref returns 0 if references remain
      if(sts)
      {
        vector_PCHAN_free( qs );
        *pqs = NULL;
      }
    }
  }

  void IDevice::_alloc_qs(vector_PCHAN **pqs, size_t n, size_t *nbuf, size_t *nbytes)
  {
    if(n){
      IDevice::_free_qs(pqs); // Release existing queues.
      *pqs = vector_PCHAN_alloc(n);
      while(n--)
        (*pqs)->contents[n] = Chan_Alloc(nbuf[n], nbytes[n]);

      (*pqs)->count = (*pqs)->nelem; // This is so we can resize correctly later.
    }
  }

  void IDevice::_alloc_qs_easy(vector_PCHAN **pqs, size_t n, size_t nbuf, size_t nbytes)
  {
    if(n){
      IDevice::_free_qs(pqs); // Release existing queues.
      *pqs = vector_PCHAN_alloc(n);
      while(n--)
        (*pqs)->contents[n] = Chan_Alloc(nbuf, nbytes);

      (*pqs)->count = (*pqs)->nelem; // This is so we can resize correctly later.
    }
  }
  // Destination channel inherits the existing channel's properties.
  // If both channels exist, the source properties are inherited.
  // One channel must exist.
  //
  // If destination channel exists, the existing one will be unref'd (deleted)
  // and replaced by the source channel (which gets ref'd).
  void
    IDevice::connect(IDevice *dst, size_t dst_channel, IDevice *src, size_t src_channel)
  { // alloc in/out channels if necessary
    Guarded_Assert( src->_out!=NULL || dst->_in!=NULL ); // can't both be NULL
    if( src->_out == NULL )
      src->_out = vector_PCHAN_alloc(src_channel + 1);
    if( dst->_in == NULL )
      dst->_in = vector_PCHAN_alloc(dst_channel + 1);
    Guarded_Assert( src->_out && dst->_in ); // neither can be NULL

    if( src_channel < src->_out->nelem )                // source channel exists
    {
      Chan *s = src->_out->contents[src_channel];

      if( dst_channel < dst->_in->nelem )
      {
        Chan **d = dst->_in->contents + dst_channel;
        s=Chan_Open(s,CHAN_NONE);
        Chan_Close(*d);
        *d=s;
      } else
      {
        vector_PCHAN_request( dst->_in, dst_channel );  // make space
        dst->_in->contents[ dst_channel ] = Chan_Open( s,CHAN_NONE );
      }
    } else if( dst_channel < dst->_in->nelem )           // dst exists, but not src
    {
      Chan *d = dst->_in->contents[dst_channel];
      vector_PCHAN_request( src->_out, src_channel );   // make space
      src->_out->contents[src_channel] = Chan_Open( d,CHAN_NONE );
    } else
    {
      error("In Agent::connect: Neither channel exists.\r\n");
    }
  }


} //end namespace fetch

