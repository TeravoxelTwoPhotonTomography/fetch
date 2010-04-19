#include "stdafx.h"
#include "agent.h"

#define DEBUG_AGENT__HANDLE_WAIT_FOR_RESULT

namespace fetch
{
    TYPE_VECTOR_DEFINE( PASYNQ );

  inline void
    Agent::lock(void)
    { EnterCriticalSection(&this->lock);
    }

  inline void
    Agent::unlock(void)
    { LeaveCriticalSection(&this->lock);
    }

    Agent(void) : 
      thread(INVALID_HANDLE_VALUE),
      _is_available(0),
      _is_running(0),
      task(NULL),
      num_waiting(0),
      in(NULL),
      out(NULL)
    { Guarded_Assert_WinErr(  
        this->notify_available = CreateEvent( NULL,   // default security attr
                                              TRUE,   // manual reset
                                              FALSE,  // initially unsignalled
                                              NULL ));// no name
      Guarded_Assert_WinErr(  
        this->notify_stop      = CreateEvent( NULL,   // default security attr
                                              FALSE,  // manual reset
                                              FALSE,  // initially unsignalled
                                              NULL ));// no name
      Guarded_Assert_WinErr(
        InitializeCriticalSectionAndSpinCount( &this->lock, 0x8000400 ));
    }

  static inline void
    _safe_free_handle( HANDLE *h )
    { if( *h != INVALID_HANDLE_VALUE )
        if( !CloseHandle(*h) )
          ReportLastWindowsError();
      *h = INVALID_HANDLE_VALUE;
    }

  static void
    Agent::_free_qs(vector_PASYNQ **pqs)
    { vector_PASYNQ *qs = *pqs;
      if( qs )
      { size_t n = qs->count;
        while(n--)
          Asynq_Unref( qs->contents[n] );
        vector_PASYNQ_free( qs );
        *pqs = NULL;
      }
    }

  static void
    Agent::_alloc_qs( vector_PASYNQ **pqs,
                      size_t  n
                      size_t *nbuf, 
                      size_t *nbytes)
    { if( num_inputs )
      { Agent::_free_qs(pqs);               // Release existing queues.
        *pqs  = vector_PASYNQ_alloc(n); 
        while( n-- )
          (*pqs)->contents[n] = Asynq_Alloc(nbuf[n],nbytes[n]);
        (*pqs)->count = (*pqs)->nelem;      // This is so we can resize correctly later.
      }
    }

  static void
    Agent::_alloc_qs_easy( vector_PASYNQ **pqs,
                           size_t n
                           size_t nbuf, 
                           size_t nbytes)
    { if( num_inputs )
      { Agent::_free_qs(pqs);               // Release existing queues.
        *pqs  = vector_PASYNQ_alloc(n); 
        while( n-- )
          (*pqs)->contents[n] = Asynq_Alloc(nbuf,nbytes);
        (*pqs)->count = (*pqs)->nelem;      // This is so we can resize correctly later.
      }
    }

    ~Agent(void)
    { 
      if(this->detach(AGENT_DEFAULT_TIMEOUT)>0)
        warning("~Agent : Attempt to detach() timed out.\r\n");

      if(this->num_waiting>0)
        warning("~Agent : Agent has waiting tasks.\r\n"
                "         Try calling Agent::detach first.\r\n.");
      
      _free_qs(&this->in);
      _free_qs(&this->out);

      _safe_free_handle( &this->thread );
      _safe_free_handle( &this->notify_available );
      _safe_free_handle( &this->notify_stop );
      
      DeleteCriticalSection( &this->lock );
      
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
  static inline unsigned
    _handle_wait_for_result(DWORD result, const char* msg)
    { return_val_if( result == WAIT_OBJECT_0, 1 );
      Guarded_Assert_WinErr( result != WAIT_FAILED );
#ifdef DEBUG_AGENT__HANDLE_WAIT_FOR_RESULT
      if( result == WAIT_ABANDONED )
        warning("Agent: Wait abandoned\r\n\t%s\r\n",msg);
      if( result == WAIT_TIMEOUT )
        warning("Agent: Wait timeout\r\n\t%s\r\n",msg);  
#endif
      return 0;
    }

  static inline unsigned
    _wait_for_available(Agent *self, DWORD timeout_ms )
    { HANDLE notify = self->notify_available;
      DWORD res;
      Guarded_Assert_WinErr( ResetEvent( notify ) );
      self->num_waiting++;
      self->unlock();
      res = WaitForSingleObject( notify, timeout_ms  );
      self->lock();
      self->num_waiting--;
      return _handle_wait_for_result(res, "Agent wait for availability.");
    }

  Agent*
    _request_available_unlocked( Agent *self, int is_try, DWORD timeout_ms )
    { if( !self->_is_available )
      { if( is_try ) return NULL;
        if( !_wait_for_available( self, timeout_ms ))
          if( !self->_is_available )
            return NULL;
      }
      return self;
    }

  unsigned int
    Agent::is_armed(void)
    { return this->task != NULL;    //task is set
    }

  unsigned int
    Agent::is_runnable(void)
    { return this->task != NULL     //task is set
            && !(this->_is_running);//but not running
    }

  unsigned int 
    Agent::is_available(void)
    { return this->_is_available;
    }

  unsigned int 
    Agent::is_running(void)
    { return this->_is_running;
    }

  void
    Agent::set_available(void)
    { //Notify waiting tasks
      this->_is_available = 1;
      if( self->num_waiting > 0 )
        Guarded_Assert_WinErr(
          SetEvent( self->notify_available ) );
    }

  unsigned int
    Agent::Arm(Task *task, DWORD timeout_ms )
    { this->lock();                                           // Source state can not be "armed" or "running"
      if( !_request_available_unlocked(this, 0, timeout_ms) ) // Blocks till agent is available
      { warning("Agent unavailable.  Perhaps another task is running?\r\n"
                "\tAborting attempt to arm.\r\n");
        goto Error;    
      }
      
      this->task = task; // save the task
      
      // Exec task config function
      if( !(task->config)(this) )
      { warning("While loading task, something went wrong with the task configuration.\r\n"
                "\tAgent not armed.\r\n");
        goto Error;
      }    
      
      // Create thread for running task
      Guarded_Assert_WinErr(
        this->thread = CreateThread(0,                  // use default access rights
                                    0,                  // use default stack size (1 MB)
                                    task->thread_main,  // main function
                                    this,               // arguments
                                    CREATE_SUSPENDED,   // don't start yet
                                    NULL ));            // don't worry about the thread id
      this->_is_available = 0;
      this->unlock();
      debug("Armed\r\n");
      return 1;
Error:
      self->task = NULL;
      this->unlock();
      return 0;
    }

  DWORD WINAPI
    _agent_arm_thread_proc( LPVOID lparam )
    { struct T {Agent *d; Task *dt; DWORD timeout;};
      asynq *q = (asynq*) lparam;
      T args = {0,0,0};

      if(!Asynq_Pop_Copy_Try(q,&args))
      { warning("In Agent::arm_nonblocking work procedure:\r\n"
                "\tCould not pop arguments from queue.\r\n");
        return 0;
      }
      return args.d->arm(args.dt,args.timeout);
    }

  BOOL
    Agent::arm_nonblocking(Task *task, DWORD timeout_ms )
  { struct T {Agent* d; Task* dt; DWORD timeout;};
    struct T args = {this,task,timeout_ms};
    static asynq *q = NULL;
    if( !q )
      q = Asynq_Alloc(32, sizeof(T) );
    if( !Asynq_Push_Copy(q, &args, TRUE /*expand queue when full*/) )
    { warning("In Agent::arm_nonblocking: Could not push request arguments to queue.");
      return 0;
    }
    return QueueUserWorkItem(&_agent_arm_thread_proc, (void*)q, NULL /*default flags*/);
  }

  unsigned int
    Agent::disarm(DWORD timeout_ms)
  { if(this->_is_running)      // Source state can be running or armed
      this->stop(timeout_ms);
    
    this->lock();
    this->task = NULL;
    this->set_available();
    this->unlock();
    
    debug("Disarmed\r\n");
    return 1;
  }

  DWORD WINAPI _agent_disarm_thread_proc( LPVOID lparam )
    { struct T {agent* d; DWORD timeout;};
      asynq *q = (asynq*) lparam;
      struct T args = {0,0};

      if(!Asynq_Pop_Copy_Try(q,&args))
      { warning("In Agent::disarm_nonblocking work procedure:\r\n"
                "\tCould not pop arguments from queue.\r\n");
        return 0;
      }
      return args.d->disarm(args.timeout);
    }

  BOOL
    Agent::disarm_nonblocking(DWORD timeout_ms)
    { struct T {Agent* d; DWORD timeout;};
      struct T args = {this,timeout_ms};
      static asynq *q = NULL;
      if( !q )
        q = Asynq_Alloc(32, sizeof(T) );
      if( !Asynq_Push_Copy(q, &args, TRUE /*expand queue when full*/) )
      { warning("In Agent::disarm_nonblocking:\r\n"
                "\tCould not push request arguments to queue.\r\n");
        return 0;
      }
      return QueueUserWorkItem(&_agent_disarm_thread_proc, (void*)q, NULL /*default flags*/);
    }

    // Transitions from armed to running state
    // Returns 1 on success
    //         0 otherwise
    // A return of 0 indicates a failure to start the thread
    // and could be due to multiple suspensions on the thread.
  unsigned int
    Agent::run(void)
    { DWORD sts = 0;  
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
                                        this->task->main,   // main function
                                        this,               // arguments
                                        0,                  // run immediately
                                        NULL ));            // don't worry about the threadid
          Guarded_Assert_WinErr(
            SetThreadPriority( this->thread,
                               THREAD_PRIORITY_TIME_CRITICAL ));
          debug("Run:4b\r\n");
        }
      } else //(then not runnable)
      { warning("Attempted to run an unarmed or already running Agent.\r\n"
                "\tAborting the run attempt.\r\n");
      }
      debug( "Agent Run: (%d) thread 0x%p\r\n", sts, this->thread );
      this->unlock();
      return sts;
    }

  DWORD WINAPI _agent_run_thread_proc( LPVOID lparam )
    { struct T {Agent* d;};
      asynq *q = (asynq*) lparam;
      struct T args = {0};

      if(!Asynq_Pop_Copy_Try(q,&args))
      { warning("In Agent::run_nonblocking work procedure:\r\n"
                "\tCould not pop arguments from queue.\r\n");
        return 0;
      }
      return args.d->run();
    }

  BOOL
    Agent::run_nonblocking()
    { struct T {Agent* d;};
      struct T args = {this};
      static asynq *q = NULL;
      return_val_if_fail( this, 0 );
      if( !q )
        q = Asynq_Alloc(32, sizeof(T) );
      if( !Asynq_Push_Copy(q, &args, TRUE /*expand queue when full*/) )
      { warning("In Agent_Run_Nonblocking:\r\n"
                "\tCould not push request arguments to queue.\r\n");
        return 0;
      }
      return QueueUserWorkItem(&_agent_run_thread_proc, (void*)q, NULL /*default flags*/);
    }

    // Transitions from running to armed state.
    // Returns 1 on success
  unsigned int
    Agent::stop(DWORD timeout_ms)
    { DWORD res;  
      this->lock();
      if( this->_is_running )
      { if( this->thread != INVALID_HANDLE_VALUE)
        { SetEvent(this->notify_stop);
          this->unlock();
          res = WaitForSingleObject(this->thread, timeout_ms); // wait for running thread to stop
          this->lock();
          
          // Handle a timeout on the wait.  
          if( !_handle_wait_for_result(res, "Agent stop: Wait for thread."))
          { warning("Timed out waiting for task thread to stop.  Forcing termination.\r\n");
            Guarded_Assert_WinErr( 
              TerminateThread(this->thread, 127) ); // Force the thread to stop
          }
          CloseHandle(this->thread);
          this->thread = INVALID_HANDLE_VALUE;
        }
        this->is_running = 0;
      }
      this->unlock();
      debug("Agent: Stopped\r\n");
      return 1;
    }

  DWORD WINAPI
    _agent_stop_thread_proc( LPVOID lparam )
    { struct T {Agent* d; DWORD timeout;};
      asynq *q = (asynq*) lparam;
      struct T args = {0,0};

      if(!Asynq_Pop_Copy_Try(q,&args))
      { warning("In Agent_Stop_Nonblocking work procedure:\r\n"
                "\tCould not pop arguments from queue.\r\n");
        return 0;
      }
      return args.d->stop(args.timeout);
    }

  BOOL
    Agent::stop_nonblocking(DWORD timeout_ms)
    { struct T {Agent* d; DWORD timeout;};
      struct T args = {this,timeout_ms};
      static asynq *q = NULL;
      if( !q )
        q = Asynq_Alloc(32, sizeof(T) );
      if( !Asynq_Push_Copy(q, &args, TRUE /*expand queue when full*/) )
      { warning("In Agent::stop_nonblocking:\r\n"
                "\tCould not push request arguments to queue.\r\n");
        return 0;
      }
      return QueueUserWorkItem(&_agent_stop_thread_proc, (void*)q, NULL /*default flags*/);
    }

  DWORD WINAPI
    _agent_detach_thread_proc( LPVOID lparam )
    { Agent *d = (Agent*) lparam;
      return args.d->detach();
    }

  BOOL
    Agent::detach_nonblocking(DWORD timeout_ms)
    { return QueueUserWorkItem(&_agent_detach_thread_proc, (void*)this, NULL /*default flags*/);
    }

  DWORD WINAPI
    _agent_attach_thread_proc( LPVOID lparam )
    { Agent *d = (Agent*) lparam;
      return args.d->attach();
    }

  BOOL
    Agent::attach_nonblocking(DWORD timeout_ms)
    { return QueueUserWorkItem(&_agent_attach_thread_proc, (void*)this, NULL /*default flags*/);
    }
/*
// FIXME: make this more of a resize op to avoid thrashing
void
DeviceTask_Alloc_Inputs( DeviceTask* self,
                             size_t  num_inputs,
                             size_t *input_queue_size, 
                             size_t *input_buffer_size)
{ if( num_inputs )
  { DeviceTask_Free_Inputs( self );
    self->in  = vector_PASYNQ_alloc( num_inputs  ); 
    while( num_inputs-- )
      self->in->contents[num_inputs] 
          = Asynq_Alloc( input_queue_size [num_inputs],
                         input_buffer_size[num_inputs] );
    self->in->count = self->in->nelem; // resizable, so use count rather than nelem alone.
  }                                    //            start full
}

// FIXME: make this more of a resize op to avoid thrashing
void
DeviceTask_Alloc_Outputs( DeviceTask* self,
                              size_t  num_outputs,
                              size_t *output_queue_size, 
                              size_t *output_buffer_size)
{ if( num_outputs )
  { DeviceTask_Free_Outputs( self );
    self->out  = vector_PASYNQ_alloc( num_outputs ); 
    while( num_outputs-- )
      self->out->contents[num_outputs] 
          = Asynq_Alloc( output_queue_size[num_outputs],
                         output_buffer_size[num_outputs] );
    self->out->count = self->out->nelem; // resizable, so use count rather than nelem alone.
  }                                      //            start full
}
*/
} //end namespace fetch

