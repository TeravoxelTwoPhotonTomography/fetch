#include "stdafx.h"
#include "device.h"

#define DEBUG_DEVICE_HANDLE_WAIT_FOR_RESULT

inline void
Device_Lock ( Device *self )
{ EnterCriticalSection( &self->lock );
}

inline void
Device_Unlock( Device *self )
{ LeaveCriticalSection( &self->lock );
}

Device*
Device_Alloc(void)
{ Device* self = (Device*)Guarded_Malloc( sizeof(Device), "Device_Alloc");
  self->num_waiting  = 0;
  self->is_available = 0;
  self->is_running   = 0;
  self->task = NULL;
  self->thread      = INVALID_HANDLE_VALUE;
  Guarded_Assert_WinErr(  
    self->notify_available = CreateEvent( NULL,   // default security attr
                                          TRUE,   // manual reset
                                          FALSE,  // initially unsignalled
                                          NULL ));// no name
  Guarded_Assert_WinErr(  
    self->notify_stop      = CreateEvent( NULL,   // default security attr
                                          FALSE,  // manual reset
                                          FALSE,  // initially unsignalled
                                          NULL ));// no name
  Guarded_Assert_WinErr(
    InitializeCriticalSectionAndSpinCount( &self->lock, 0x8000400 ));
   
  return self;  
}

static inline void
_safe_free_handle( HANDLE *h )
{ if( *h != INVALID_HANDLE_VALUE )
    if( !CloseHandle(*h) )
      ReportLastWindowsError();
  *h = INVALID_HANDLE_VALUE;
}

void
Device_Free(Device* self)
{ return_if_fail(self);
  if(self->num_waiting>0)
    warning("Device Free: Device has waiting tasks.\r\n"
            "             Try calling Device_Release first.\r\n.");
  
  _safe_free_handle( &self->thread );
  _safe_free_handle( &self->notify_available );
  _safe_free_handle( &self->notify_stop );
  
  DeleteCriticalSection( &self->lock );
  
  self->task = NULL;
  free(self);
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
#ifdef DEBUG_DEVICE_HANDLE_WAIT_FOR_RESULT
  if( result != WAIT_ABANDONED )                   // This is the common timeout result
    warning("Device: Wait abandoned\r\n\t%s\r\n",msg);
  if( result != WAIT_TIMEOUT )                     // Don't know how to generate this.
    warning("Device: Wait timeout\r\n\t%s\r\n",msg);  
#endif
  return 0;
}

static inline unsigned
_device_wait_for_available(Device *self, DWORD timeout_ms )
{ HANDLE notify = self->notify_available;
  DWORD res;
  Guarded_Assert_WinErr( ResetEvent( notify ) );
  self->num_waiting++;
  Device_Unlock(self);
  res = WaitForSingleObject( notify, timeout_ms  );
  Device_Lock(self);
  self->num_waiting--;
  return _handle_wait_for_result(res, "Device wait for availability.");
}

Device*
_device_request_available_unlocked( Device *self, int is_try, DWORD timeout_ms )
{ if( !self->is_available )
  { if( is_try ) return NULL;
    if( !_device_wait_for_available( self, timeout_ms ))
      if( !self->is_available )
        return NULL;
  }
  return self;
}

unsigned int
Device_Is_Armed( Device *self )
{ return_val_if_fail( self, 0 );
  return self->task != NULL           //task is set
         && !Device_Is_Running(self); //but not running
}

unsigned int 
Device_Is_Running( Device *self )
{ return_val_if_fail( self, 0 );
  return self->is_running;
}

unsigned int
Device_Arm ( Device *self, DeviceTask *task, DWORD timeout_ms )
{ return_val_if_fail( self, 0 );
  Device_Lock(self);                                                         // Source state can not be "armed" or "running"
  if( !_device_request_available_unlocked(self, 0, timeout_ms) )             // Blocks till device is available
  { warning("Device unavailable.  Perhaps another task is running?\r\n"
            "\tAborting attempt to arm.\r\n");
    goto Error;    
  }
  
  self->task = task; // save the task
  
  // Exec device config function
  if( !(task->cfg_proc)(self, task->in, task->out) )
  { warning("While loading task, something went wrong with the device configuration.\r\n"
            "\tDevice not armed.\r\n");
    goto Error;
  }    
  
  // Create thread for running task
  Guarded_Assert_WinErr(
    self->thread = CreateThread( 0,                  // use default access rights
                                 0,                  // use default stack size (1 MB)
                                 task->main,         // main function
                                 self,               // arguments
                                 CREATE_SUSPENDED,   // don't start yet
                                 NULL ));            // don't worry about the threadid
  self->is_available = 0;                                 
  Device_Unlock(self);
  debug("Armed\r\n");
  return 1;
Error:
  self->task = NULL;
  Device_Unlock(self);
  return 0;
}

DWORD WINAPI _device_arm_thread_proc( LPVOID lparam )
{ struct T {Device *d; DeviceTask *dt; DWORD timeout;};
  asynq *q = (asynq*) lparam;
  T args = {0,0,0};

  if(!Asynq_Pop_Copy_Try(q,&args))
  { warning("In Device_Arm_Nonblocking work procedure:\r\n"
            "\tCould not pop arguments from queue.\r\n"
            "\tThis means a request got lost somewhere.\r\n");
    return 0;
  }
  return Device_Arm( args.d, args.dt, args.timeout );
}

BOOL
Device_Arm_Nonblocking( Device *self, DeviceTask *task, DWORD timeout_ms )
{ struct T {Device* d; DeviceTask* dt; DWORD timeout;};
  struct T args = {self,task,timeout_ms};
  static asynq *q = NULL;
  return_val_if_fail( self, 0 );
  if( !q )
    q = Asynq_Alloc(32, sizeof(T) );
  if( !Asynq_Push_Copy(q, &args, TRUE /*expand queue when full*/) )
  { warning("In Device_Arm_Nonblocking: Could not push request arguments to queue.");
    return 0;
  }
  return QueueUserWorkItem(&_device_arm_thread_proc, (void*)q, NULL /*default flags*/);
}

unsigned int
Device_Disarm( Device *self, DWORD timeout_ms )
{ return_val_if_fail( self, 0 );
  Device_Lock(self);
  
  if( Device_Is_Running(self) )                     // Source state can be running or armed
    Device_Stop(self, timeout_ms);
    
  self->task = NULL;    //Unreference task
  
  //Notify waiting tasks
  self->is_available = 1;
  if( self->num_waiting > 0 )
    Guarded_Assert_WinErr( 
      SetEvent( self->notify_available ) );  

  Device_Unlock(self);
  debug("Disarmed\r\n");
  return 1;
}

DWORD WINAPI _device_disarm_thread_proc( LPVOID lparam )
{ struct T {Device* d; DWORD timeout;};
  asynq *q = (asynq*) lparam;
  struct T args = {0,0};

  if(!Asynq_Pop_Copy_Try(q,&args))
  { warning("In Device_Disarm_Nonblocking work procedure:\r\n"
            "\tCould not pop arguments from queue.\r\n"
            "\tThis means a request got lost somewhere.\r\n");
    return 0;
  }
  return Device_Disarm( args.d, args.timeout );
}

BOOL
Device_Disarm_Nonblocking( Device *self, DWORD timeout_ms )
{ struct T {Device* d; DWORD timeout;};
  struct T args = {self,timeout_ms};
  static asynq *q = NULL;
  return_val_if_fail( self, 0 );
  if( !q )
    q = Asynq_Alloc(32, sizeof(T) );
  if( !Asynq_Push_Copy(q, &args, TRUE /*expand queue when full*/) )
  { warning("In Device_Disarm_Nonblocking:\r\n"
            "\tCould not push request arguments to queue.\r\n");
    return 0;
  }
  return QueueUserWorkItem(&_device_disarm_thread_proc, (void*)q, NULL /*default flags*/);
}

// Transitions from armed to running state
// Returns 1 on success
//         0 otherwise
// A return of 0 indicates a failure to start the thread
// and could be due to multiple suspensions on the thread.
unsigned int
Device_Run ( Device *self )
{ DWORD sts = 0;  
  return_val_if_fail( self, 0 );  
  Device_Lock(self);  
  if( Device_Is_Armed(self) )      // Source state condition: Must be armed.
  { self->is_running = 1;    
    if( self->thread != INVALID_HANDLE_VALUE )
    { sts = ResumeThread(self->thread) <= 1; // Thread's already alloced so go!      
    }
    else
    { // Create thread for running the task
      Guarded_Assert_WinErr(
        self->thread = CreateThread( 0,                  // use default access rights
                                     0,                  // use default stack size (1 MB)
                                     self->task->main,   // main function
                                     self,               // arguments
                                     0,                  // run immediately
                                     NULL ));            // don't worry about the threadid
      debug("Run:4b\r\n");
    }
  } else
  { warning("Attempted to run an unarmed device.\r\n"
            "\tAborting the run attempt.\r\n");
  }
  debug( "Device Run: (%d) thread 0x%p\r\n", sts, self->thread );
  Device_Unlock(self);
  return sts;
}

DWORD WINAPI _device_run_thread_proc( LPVOID lparam )
{ struct T {Device* d;};
  asynq *q = (asynq*) lparam;
  struct T args = {0};

  if(!Asynq_Pop_Copy_Try(q,&args))
  { warning("In Device_Run_Nonblocking work procedure:\r\n"
            "\tCould not pop arguments from queue.\r\n"
            "\tThis means a request got lost somewhere.\r\n");
    return 0;
  }
  return Device_Run( args.d );
}

BOOL
Device_Run_Nonblocking( Device *self )
{ struct T {Device* d;};
  struct T args = {self};
  static asynq *q = NULL;
  return_val_if_fail( self, 0 );
  if( !q )
    q = Asynq_Alloc(32, sizeof(T) );
  if( !Asynq_Push_Copy(q, &args, TRUE /*expand queue when full*/) )
  { warning("In Device_Run_Nonblocking:\r\n"
            "\tCould not push request arguments to queue.\r\n");
    return 0;
  }
  return QueueUserWorkItem(&_device_run_thread_proc, (void*)q, NULL /*default flags*/);
}

// Transitions from running to armed state.
// Returns 1 on success
unsigned int
Device_Stop( Device *self, DWORD timeout_ms )
{ DWORD res;  
  return_val_if_fail( self, 0 );
  Device_Lock(self);
  if( self->is_running )                                       // Source state condition (must be running)
  { //Terminate thread
    if( self->thread != INVALID_HANDLE_VALUE)
    { SetEvent(self->notify_stop);                             // signal task proc to quit
      Device_Unlock(self);
      res = WaitForSingleObject(self->thread, timeout_ms);     // wait to quit
      Device_Lock(self);
      
      // Handle a timeout on the wait.  
      if( !_handle_wait_for_result(res, "Device Release: Wait for thread."))
      { warning("Timed out waiting for task thread to stop.  Forcing termination.\r\n");
        Guarded_Assert_WinErr( 
          TerminateThread(self->thread, 127) ); // Force the thread to stop
      }
      CloseHandle(self->thread);
      self->thread = INVALID_HANDLE_VALUE;
    }
    
    self->is_running = 0;
  }
  Device_Unlock(self);
  debug("Device: Stopped\r\n");
  return 1;
}

DWORD WINAPI _device_stop_thread_proc( LPVOID lparam )
{ struct T {Device* d; DWORD timeout;};
  asynq *q = (asynq*) lparam;
  struct T args = {0,0};

  if(!Asynq_Pop_Copy_Try(q,&args))
  { warning("In Device_Stop_Nonblocking work procedure:\r\n"
            "\tCould not pop arguments from queue.\r\n"
            "\tThis means a request got lost somewhere.\r\n");
    return 0;
  }
  return Device_Stop( args.d, args.timeout );
}

BOOL
Device_Stop_Nonblocking( Device *self, DWORD timeout_ms )
{ struct T {Device* d; DWORD timeout;};
  struct T args = {self,timeout_ms};
  static asynq *q = NULL;
  return_val_if_fail( self, 0 );
  if( !q )
    q = Asynq_Alloc(32, sizeof(T) );
  if( !Asynq_Push_Copy(q, &args, TRUE /*expand queue when full*/) )
  { warning("In Device_Stop_Nonblocking:\r\n"
            "\tCould not push request arguments to queue.\r\n");
    return 0;
  }
  return QueueUserWorkItem(&_device_stop_thread_proc, (void*)q, NULL /*default flags*/);
}