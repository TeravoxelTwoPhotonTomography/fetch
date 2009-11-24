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
  self->num_waiting = 0;
  self->is_available = 0;
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
_device_wait_for_unlocked(Device *self, int is_try, DWORD timeout_ms )
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
_device_request( Device *self, int is_try, DWORD timeout_ms )
{ 
  Device_Lock(self);
  if( !self->is_available )
  { if( is_try ) return NULL;
    if( !_device_wait_for_unlocked( self, is_try, timeout_ms ))
      if( !self->is_available )
        return NULL;
  }
  self->is_available = 0;
  Device_Unlock(self);
  return self;
}

Device*
Device_Request( Device *self, DWORD timeout_ms )
{ return _device_request( self,0,timeout_ms );
}

Device*
Device_Request_Try( Device *self )
{ return _device_request( self,1,INFINITE );
}

// Transitions device to "Hold" state
// In hold state, device can be loaded with a new task.
// Returns 1 on success, 0 otherwise
unsigned int
Device_Release( Device *self, DWORD timeout_ms )
{ DWORD res;
  unsigned int ret;

  //Terminate thread
  return_val_if( self->thread == INVALID_HANDLE_VALUE, 1); // No task loaded, so return.
  SetEvent(self->notify_stop);                             // signal task proc to quit
  res = WaitForSingleObject(self->thread, timeout_ms);     // wait
  
  Device_Lock(self);
  { if( !(ret=_handle_wait_for_result(res, "Device Release: Wait for thread.")) )
    { warning("Timed out waiting for task thread to stop.  Forcing termination.\r\n");
      Guarded_Assert_WinErr( 
        TerminateThread(self->thread, 127) );
    }
    Guarded_Assert_WinErr( 
      CloseHandle(self->thread) );
    self->thread = INVALID_HANDLE_VALUE;
    self->task = NULL;

    //Notify waiting tasks
    self->is_available = 1;
    if( self->num_waiting > 0 )
      Guarded_Assert_WinErr( 
        SetEvent( self->notify_available ) );
  }
  Device_Unlock(self);
  return 1;
}

unsigned int
Device_Arm ( Device *self, DeviceTask *task )
{ Device_Lock(self);
  if( !self->is_available )
  { warning("Attempted to arm an unavailable device.\r\n");
    goto Error;
  }
  // Exec device config function
  goto_if_fail( (task->cfg_proc)(self), Error );
  self->task = task;
  
  // Create thread for running task
  Guarded_Assert_WinErr(
    self->thread = CreateThread( 0,                  // use default access rights
                                 0,                  // use default stack size (1 MB)
                                 task->main,         // main function
                                 &self,              // arguments
                                 CREATE_SUSPENDED,   // don't start yet
                                 NULL ));            // don't worry about the threadid
  self->is_available = 0;                                 
  Device_Unlock(self);
  return 1;
Error:
  Device_Unlock(self);
  return 0;
}

unsigned int
Device_Run ( Device *self )
{ Guarded_Assert( self->thread != INVALID_HANDLE_VALUE );
  return ResumeThread(self->thread) <= 1;
}

unsigned int
Device_Stop( Device *self )
{ return SetEvent(self->notify_stop);
}