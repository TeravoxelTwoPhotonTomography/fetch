#include "stdafx.h"
#include "asynq.h"

asynq*
Asynq_Alloc(size_t buffer_count, size_t buffer_size_bytes )
{ asynq *self = Guarded_Malloc( sizeof(asynq), "Asynq_Alloc" );
  
  self->q = RingFIFO_Alloc( buffer_count, buffer_size_bytes );
  
  self->ref_count       = 1;
  self->waiting_producer_threads = 0;
  self->waiting_consumer_threads = 0;

  // For spin count,
  //    set high bit to preallocate event used in EnterCriticalSection for W2K
  //    see: http://msdn.microsoft.com/en-us/library/ms683472(VS.85).aspx
  Guarded_Assert_WinErr( 
    InitializeCriticalSectionAndSpinCount( &self->lock, 
                                            0x80000400 ) 
  );
  Guarded_Assert_WinErr(
    self->notify_data = CreateEvent(NULL,   // default security attributes
                                    TRUE,   // true ==> manual reset
                                    FALSE,  // initial state is untriggered
                                    NULL ); // unnamed
  );
  Guarded_Assert_WinErr(
    self->notify_space = CreateEvent( NULL,   // default security attributes
                                      TRUE,   // true ==> manual reset
                                      TRUE,   // initial state is triggered
                                      NULL ); // unnamed
  );  

  return self;
}

void 
Asynq_Ref( asynq *self )
{ return_if_fail( self );  
  return_if_fail( self->ref_count > 0 );
  
  Interlocked_Inc_u32( &self->ref_count );
}

void 
Asynq_Unref( asynq *self )
{ return_if_fail( self );  
  return_if_fail( self->ref_count > 0 );
  
  if( Interlocked_Dec_And_Test_u32( &self->ref_count ) )              // Free when last reference is released
  { return_if_fail( self->waiting_producer_threads == 0 );
    return_if_fail( self->waiting_consumer_threads == 0 );            // Gaurantee no one is waiting
    DeleteCriticalSection( &self->lock );
    if( !CloseHandle( self->notify_data  )) ReportLastWindowsError();
    if( !CloseHandle( self->notify_space )) ReportLastWindowsError();
    if( self->q )
      RingFIFO_Free( self->q );
	  free(self);
	}
}

inline void 
Asynq_Lock ( asynq *self )
{ EnterCriticalSection( &self->lock );
}

/* Returns 1 on success, 0 otherwise.
 * Possibly generates a panic shutdown.
 * Lack of success could indicate the lock was
 *   abandoned or a timeout elapsed.
 * A warning will be generated if the lock was
 *   abandoned.
 */
static inline unsigned
_handle_wait_for_result(DWORD result, const char* msg)
{ return_val_if( result == WAIT_OBJECT_0, 1 );
  Guarded_Assert_WinErr( result != WAIT_FAILED );
  if( result != WAIT_ABANDONED )
    warning("Wait abandoned\r\n\t%s\r\n",msg);
  return 0;
}

inline void 
Asynq_Unlock ( asynq *self )
{ LeaveCriticalSection( &self->lock );
}

static unsigned int
_asynq_push_unlocked(                        // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void **pbuf,           // Pointer to the input buffer.  On return from a success, *pbuf points to different but valid buffer for writing.
                      int expand_on_full,    // Allocate additional space in the queue if the queue is full, and then push.
                      int try,               // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ unsigned int success = 0,
               full    = 0;
  if( try )
    success = ( (full = RingFIFO_Push_Try(self->q, pbuf)) == 0); // push succeeded if return indicates queue is not full
  else 
  { success = 1; 
    if( RingFIFO_Push( self, pbuf, expand_on_full ) && !expand_on_full )
    { success = _handle_wait_for_result(
                    WaitForSingleObject( self->notify_space, timeout_ms ),
                    "AsynQ Push");
    }
  }
  // Notify data available
  if( success && self->waiting_consumer_threads )
    Guarded_Assert_WinErr( SetEvent( self->notify_data ) );
  return success;
}

unsigned int
Asynq_Push( asynq *self, void **pbuf, int expand_on_full )
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_push_unlocked(self, pbuf, expand_on_full, FALSE, INFINITE );
  Asynq_Unlock();
  return result;
}