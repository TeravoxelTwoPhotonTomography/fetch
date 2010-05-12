#include "stdafx.h"
#include "agent.h"
#include "task.h"

#define DEBUG_AGENT__HANDLE_WAIT_FOR_RESULT

#if 1
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG(...)
#endif

namespace fetch
{
    TYPE_VECTOR_DEFINE( PASYNQ );

  inline void
    Agent::lock(void)
    { EnterCriticalSection(&_lock);
    }

  inline void
    Agent::unlock(void)
    { LeaveCriticalSection(&_lock);
    }

    Agent::Agent(void) :
      thread(INVALID_HANDLE_VALUE),
      _is_available(0),
      _is_running(0),
      task(NULL),
      num_waiting(0),
      in(NULL),
      out(NULL)
    {
        // default security attr
        // manual reset
        // initially unsignalled
        Guarded_Assert_WinErr(  
        this->notify_available = CreateEvent( NULL,   // default security attr
                                              TRUE,   // manual reset
                                              FALSE,  // initially unsignalled
                                              NULL ));
        // default security attr
        // manual reset
        // initially unsignalled
        Guarded_Assert_WinErr(  
        this->notify_stop      = CreateEvent( NULL,   // default security attr
                                              FALSE,  // manual reset
                                              FALSE,  // initially unsignalled
                                              NULL ));
        Guarded_Assert_WinErr(
        InitializeCriticalSectionAndSpinCount( &_lock, 0x8000400 ));
    }

    inline static void _safe_free_handle(HANDLE *h)
    {
        if(*h != INVALID_HANDLE_VALUE)
            if(!CloseHandle(*h))
                ReportLastWindowsError();


        *h = INVALID_HANDLE_VALUE;
    }
    
    void Agent::_free_qs(vector_PASYNQ **pqs)
    {
      vector_PASYNQ *qs = *pqs;
      if( qs )
      { size_t n = qs->count;
        int sts = 1;
        while(n--)
          sts &= Asynq_Unref( qs->contents[n] );      //qs[n] isn't necessarily deleted. Unref returns 0 if references remain
        if(sts)
        { vector_PASYNQ_free( qs );
          *pqs = NULL;
        }
      }
    }

    void Agent::_alloc_qs(vector_PASYNQ **pqs, size_t n, size_t *nbuf, size_t *nbytes)
    {
        if(n){
            Agent::_free_qs(pqs); // Release existing queues.
            *pqs = vector_PASYNQ_alloc(n);
            while(n--)
                (*pqs)->contents[n] = Asynq_Alloc(nbuf[n], nbytes[n]);

            (*pqs)->count = (*pqs)->nelem; // This is so we can resize correctly later.
        }
    }

    void Agent::_alloc_qs_easy(vector_PASYNQ **pqs, size_t n, size_t nbuf, size_t nbytes)
    {
        if(n){
            Agent::_free_qs(pqs); // Release existing queues.
            *pqs = vector_PASYNQ_alloc(n);
            while(n--)
                (*pqs)->contents[n] = Asynq_Alloc(nbuf, nbytes);

            (*pqs)->count = (*pqs)->nelem; // This is so we can resize correctly later.
        }
    }

    Agent::~Agent(void)
    {
        //if(this->detach() > 0)                                       // FIXME: This doesn't work
        //    warning("~Agent : Attempt to detach() timed out.\r\n");

        if(this->num_waiting > 0)
            warning("~Agent : Agent has waiting tasks.\r\n         Try calling Agent::detach first.\r\n.");

        _free_qs(&this->in);
        _free_qs(&this->out);
        _safe_free_handle(&this->thread);
        _safe_free_handle(&this->notify_available);
        _safe_free_handle(&this->notify_stop);
        DeleteCriticalSection(&this->_lock);
        this->task = NULL;
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
    
    inline unsigned Agent::_wait_for_available(DWORD timeout_ms)
    {
        HANDLE notify = this->notify_available;
        DWORD res;
        Guarded_Assert_WinErr( ResetEvent( notify ) );
        this->num_waiting++;
        this->unlock();
        res = WaitForSingleObject(notify, timeout_ms);
        this->lock();
        this->num_waiting--;
        return _handle_wait_for_result(res, "Agent wait for availability.");
    }

    Agent* Agent::_request_available_unlocked(int is_try, DWORD timeout_ms)
    {
      if( !this->_is_available )
      { if( is_try ) return NULL;
        if( !_wait_for_available(timeout_ms))
          if( !this->_is_available )
            return NULL;
      }
      return this;
    }

    unsigned int Agent::is_attached(void)
    { return is_available() || this->task != NULL;
    }

    unsigned int Agent::is_armed(void)
    {
        return this->task != NULL;
    }

    unsigned int Agent::is_runnable(void)
    {
        return this->task != NULL     //task is set
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
    { return WAIT_OBJECT_0 == WaitForSingleObject(notify_stop, 0);
    }

    void Agent::set_available(void)
    {
        //Notify waiting tasks
        this->_is_available = 1;
        if( this->num_waiting > 0 )
        Guarded_Assert_WinErr(
          SetEvent( this->notify_available ) )
        ;
    }

    unsigned int Agent::arm(Task *task, DWORD timeout_ms)
    {
      this->lock(); // Source state can not be "armed" or "running"
      if(!_request_available_unlocked(0/*try?*/, timeout_ms))// Blocks till agent is available
      {
          warning("Agent unavailable.  Perhaps another task is running?\r\n\tAborting attempt to arm.\r\n");
          goto Error;
      }
      this->task = task; // save the task
      // Exec task config function
      if(!(task->config)(this)){
          warning("While loading task, something went wrong with the task configuration.\r\n\tAgent not armed.\r\n");
          goto Error;
      }
      // Create thread for running task
      // use default access rights
      // use default stack size (1 MB)
      // main function
      // arguments
      // don't start yet
      Guarded_Assert_WinErr(
      this->thread = CreateThread(0,                  // use default access rights
                                  0,                  // use default stack size (1 MB)
                                  task->thread_main,  // main function
                                  this,               // arguments
                                  CREATE_SUSPENDED,   // don't start yet
                                  NULL )); // don't worry about the thread id
      this->_is_available = 0;
      this->unlock();
      DBG("Armed 0x%p\r\n",this);
      return 1;
    Error:
      this->task = NULL;
      this->unlock();
      return 0;
    }

    DWORD WINAPI _agent_arm_thread_proc(LPVOID lparam)
    {
        struct T
        {
            Agent *d;
            Task *dt;
            DWORD timeout;
        };
        asynq *q = (asynq*)(lparam);
        T args = {0, 0, 0};
        if(!Asynq_Pop_Copy_Try(q, &args, sizeof(T))){
            warning("In Agent::arm_nonblocking work procedure:\r\n\tCould not pop arguments from queue.\r\n");
            return 0;
        }
        return args.d->arm(args.dt, args.timeout);
    }

    BOOL Agent::arm_nonblocking(Task *task, DWORD timeout_ms)
    {
        struct T
        {
            Agent *d;
            Task *dt;
            DWORD timeout;
        };
        struct T args = {this, task, timeout_ms};
        static asynq *q = NULL;
        if(!q)
            q = Asynq_Alloc(32, sizeof (T));

        if(!Asynq_Push_Copy(q, &args, sizeof(T), TRUE)){
            warning("In Agent::arm_nonblocking: Could not push request arguments to queue.");
            return 0;
        }
        return QueueUserWorkItem(&_agent_arm_thread_proc, (void*)q, NULL /*default flags*/);
    }

    unsigned int Agent::disarm(DWORD timeout_ms)
    {
        if(this->_is_running)
            // Source state can be running or armed
            this->stop(timeout_ms);

        this->lock();
        this->task = NULL;
        this->set_available();
        this->unlock();
        DBG("Disarmed 0x%p\r\n",this);
        return 1;
    }
    
    DWORD WINAPI _agent_disarm_thread_proc(LPVOID lparam)
    {
        struct T
        {
            Agent *d;
            DWORD timeout;
        };
        asynq *q = (asynq*)(lparam);
        struct T args = {0, 0};
        if(!Asynq_Pop_Copy_Try(q, &args, sizeof(T))){
            warning("In Agent::disarm_nonblocking work procedure:\r\n\tCould not pop arguments from queue.\r\n");
            return 0;
        }
        return args.d->disarm(args.timeout);
    }

    BOOL Agent::disarm_nonblocking(DWORD timeout_ms)
    {
        struct T
        {
            Agent *d;
            DWORD timeout;
        };
        struct T args = {this, timeout_ms};
        static asynq *q = NULL;
        if(!q)
            q = Asynq_Alloc(32, sizeof (T));

        if(!Asynq_Push_Copy(q, &args, sizeof(T), TRUE)){
            warning("In Agent::disarm_nonblocking:\r\n\tCould not push request arguments to queue.\r\n");
            return 0;
        }
        return QueueUserWorkItem(&_agent_disarm_thread_proc, (void*)q, NULL /*default flags*/);
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
      { this->_is_running = 1;    
        if( this->thread != INVALID_HANDLE_VALUE )
        { sts = ResumeThread(this->thread) <= 1; // Thread's already allocated so go!      
        }
        else
        { // Create thread for running the task
          Guarded_Assert_WinErr(
            this->thread = CreateThread(0,                  // use default access rights
                                        0,                  // use default stack size (1 MB)
                                        this->task->thread_main, // main function
                                        this,               // arguments
                                        0,                  // run immediately
                                        NULL ));            // don't worry about the threadid
#if 0
          Guarded_Assert_WinErr(
            SetThreadPriority( this->thread,
                               THREAD_PRIORITY_TIME_CRITICAL ));
#endif
          DBG("Run:4b\r\n");
        }
      } else //(then not runnable)
      { warning("Attempted to run an unarmed or already running Agent.\r\n"
                "\tAborting the run attempt.\r\n");
      }
        DBG("Agent Run: 0x%p (sts %d) thread 0x%p\r\n", this, sts, this->thread);
        this->unlock();
        return sts;
    }

    DWORD WINAPI _agent_run_thread_proc(LPVOID lparam)
    {
        struct T
        {
            Agent *d;
        };
        asynq *q = (asynq*)(lparam);
        struct T args = {0};
        if(!Asynq_Pop_Copy_Try(q, &args, sizeof(T))){
            warning("In Agent::run_nonblocking work procedure:\r\n\tCould not pop arguments from queue.\r\n");
            return 0;
        }
        return args.d->run();
    }

    BOOL Agent::run_nonblocking()
    {
        struct T
        {
            Agent *d;
        };
        struct T args = {this};
        static asynq *q = NULL;
        return_val_if_fail( this, 0 );
        if(!q)
            q = Asynq_Alloc(32, sizeof (T));

        if(!Asynq_Push_Copy(q, &args, sizeof(T), TRUE)){
            warning("In Agent_Run_Nonblocking:\r\n\tCould not push request arguments to queue.\r\n");
            return 0;
        }
        return QueueUserWorkItem(&_agent_run_thread_proc, (void*)q, NULL /*default flags*/);
    }

    // Transitions from running to armed state.
    // Returns 1 on success
    unsigned int Agent::stop(DWORD timeout_ms)
    {
      DWORD res;
      this->lock();
      if( this->_is_running )
      { if( this->thread != INVALID_HANDLE_VALUE)
        { SetEvent(this->notify_stop);
          this->unlock();
          res = WaitForSingleObject(this->thread, timeout_ms); // wait for running thread to stop
          this->lock();

          // Handle a timeout on the wait.  
          if( !_handle_wait_for_result(res, "Agent stop: Wait for thread."))
          { warning("Timed out waiting for task thread (0x%p) to stop.  Forcing termination.\r\n",this->thread);
            Guarded_Assert_WinErr(TerminateThread(this->thread,127)); // Force the thread to stop
          } 
          CloseHandle(this->thread);
          this->thread = INVALID_HANDLE_VALUE;
        } 
        this->_is_running = 0;
      }
      this->unlock();
      DBG("Agent: Stopped 0x%p\r\n",this);
      return 1;
    }

    DWORD WINAPI _agent_stop_thread_proc(LPVOID lparam)
    {
        struct T
        {
            Agent *d;
            DWORD timeout;
        };
        asynq *q = (asynq*)(lparam);
        struct T args = {0, 0};
        if(!Asynq_Pop_Copy_Try(q, &args, sizeof(T))){
            warning("In Agent_Stop_Nonblocking work procedure:\r\n\tCould not pop arguments from queue.\r\n");
            return 0;
        }
        return args.d->stop(args.timeout);
    }

    BOOL Agent::stop_nonblocking(DWORD timeout_ms)
    {
        struct T
        {
            Agent *d;
            DWORD timeout;
        };
        struct T args = {this, timeout_ms};
        static asynq *q = NULL;
        if(!q)
            q = Asynq_Alloc(32, sizeof (T));

        if(!Asynq_Push_Copy(q, &args, sizeof(T), TRUE)){
            warning("In Agent::stop_nonblocking:\r\n\tCould not push request arguments to queue.\r\n");
            return 0;
        }
        return QueueUserWorkItem(&_agent_stop_thread_proc, (void*)q, NULL /*default flags*/);
    }

    DWORD WINAPI _agent_detach_thread_proc(LPVOID lparam)
    {
        Agent *d = (Agent*)(lparam);
        return d->detach();
    }

    BOOL Agent::detach_nonblocking()
    {
        return QueueUserWorkItem(&_agent_detach_thread_proc, (void*)this, NULL /*default flags*/);
    }

    DWORD WINAPI _agent_attach_thread_proc(LPVOID lparam)
    {
      Agent *d = (Agent*)(lparam);
      return d->attach();
    }

  BOOL
    Agent::attach_nonblocking()
    { return QueueUserWorkItem(&_agent_attach_thread_proc, (void*)this, NULL /*default flags*/);
    }

  // Destination channel inherits the existing channel's properties.
  // If both channels exist, the source properties are inherited.
  // One channel must exist.
  // 
  // If destination channel exists, the existing one will be unref'd (deleted)
  // and replaced by the source channel (which gets ref'd).
  void
    Agent::connect(Agent *dst, size_t dst_channel, Agent *src, size_t src_channel)
  { // alloc in/out channels if neccessary
    if( src->out == NULL )
      src->out = vector_PASYNQ_alloc(src_channel + 1);
    if( dst->in == NULL )
      dst->in = vector_PASYNQ_alloc(dst_channel + 1);
    Guarded_Assert( src->out && dst->in );

    if( src_channel < src->out->nelem )                // source channel exists
    { asynq *s = src->out->contents[src_channel];

      if( dst_channel < dst->in->nelem )
      { asynq **d = dst->in->contents + dst_channel;
        Asynq_Ref(s);
        Asynq_Unref(*d);
        *d=s;
      } else
      { vector_PASYNQ_request( dst->in, dst_channel );  // make space
        dst->in->contents[ dst_channel ] = Asynq_Ref( s );
      }
    } else if( dst_channel < dst->in->nelem )           // dst exists, but not src
    { asynq *d = dst->in->contents[dst_channel];
      vector_PASYNQ_request( src->out, src_channel );   // make space
      src->out->contents[src_channel] = Asynq_Ref( d );
    } else
    { error("In Agent::connect: Neither channel exists.\r\n");
    }
  }


} //end namespace fetch

