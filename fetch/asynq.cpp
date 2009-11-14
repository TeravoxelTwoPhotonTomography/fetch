#include "stdafx.h"
#include "asynq.h"

asynq*
Asynq_Alloc(size_t buffer_count, size_t buffer_size_bytes )
{ asynq *self = Guarded_Malloc( sizeof(asynq), "Asynq_Alloc" );
  
  self->q = RingFIFO_Alloc( buffer_count, buffer_size_bytes );
  
  self->ref_count       = 1;
  self->waiting_producers = 0;
  self->waiting_consumers = 0;

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
  { return_if_fail( self->waiting_producers == 0 );
    return_if_fail( self->waiting_consumers == 0 );            // Gaurantee no one is waiting
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

//
// Fundamental pop/push behavior
//

static unsigned int
_asynq_push_unlocked(                        // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void **pbuf,           // Pointer to the input buffer.  On return from a success, *pbuf points to different but valid buffer for writing.
                      int expand_on_full,    // Allocate additional space in the queue if the queue is full, and then push.
                      int try,               // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ HANDLE notify = self->notify_space;
  if( RingFIFO_Is_Full( self->q ) && !expand_on_full)
  { if(try) return 0;
  
    Guarded_Assert_WinErr( ResetEvent( notify ) );
    self->waiting_producers++;
    while( RingFIFO_Is_Full( self->q ) )
      if( !_handle_wait_for_result( 
                  WaitForSingleObject( notify, timeout_ms ), 
                  "Asynq Push") )
        break; 
    self->waiting_producers--;
    if( RingFIFO_Is_Full( self->q ) )
      return 0;    
  }
  RingFIFO_Push( self->q, pbuf, expand_on_full );
  if( self->waiting_consumers )
    SetEvent( self->notify_data );
  return 1;
}

static unsigned int
_asynq_pop_unlocked(                         // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void **pbuf,           // Pointer to the input buffer.  On return from a success, *pbuf points to different but valid buffer for writing.
                      int try,               // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ HANDLE notify = self->notify_data;
  if( RingFIFO_Is_Empty( self->q ))
  { if(try) return 0;
  
    Guarded_Assert_WinErr( ResetEvent( notify ) );
    self->waiting_consumers++;
    while( RingFIFO_Is_Empty( self->q ) )
      if( !_handle_wait_for_result( 
                  WaitForSingleObject( notify, timeout_ms ), 
                  "Asynq Pop") )
        break; 
    self->waiting_consumers--;
    if( RingFIFO_Is_Empty( self->q ) )
      return 0;    
  }
  RingFIFO_Pop( self->q, pbuf );
  if( self->waiting_producers )
    SetEvent( self->notify_space );
  return 1;
}

static unsigned int
_asynq_peek_unlocked(                        // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void *buf,             // Destination buffer.
                      int try,               // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ HANDLE notify = self->notify_data;
  if( RingFIFO_Is_Empty( self->q ))
  { if(try) return 0;
  
    Guarded_Assert_WinErr( ResetEvent( notify ) );
    self->waiting_consumers++;
    while( RingFIFO_Is_Empty( self->q ) )
      if( !_handle_wait_for_result( 
                  WaitForSingleObject( notify, timeout_ms ), 
                  "Asynq Peek") )
        break; 
    self->waiting_consumers--;
    if( RingFIFO_Is_Empty( self->q ) )
      return 0;    
  }
  RingFIFO_Peek( self->q, buf );

  return 1;
}

//
// Push
//

unsigned int
Asynq_Push( asynq *self, void **pbuf, int expand_on_full )
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_push_unlocked(self, pbuf, expand_on_full, FALSE, INFINITE );
  Asynq_Unlock();
  return result;
}

unsigned int
Asynq_Push_Try( asynq *self, void **pbuf )
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_push_unlocked(self, pbuf, 
                                FALSE,      // expand on full
                                TRUE,       // try
                                INFINITE ); // timeout
  Asynq_Unlock();
  return result;
}

unsigned int
Asynq_Push_Timed( asynq *self, void **pbuf, DWORD timeout_ms )
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_push_unlocked(self, pbuf, 
                                FALSE,        // expand on full
                                FALSE,        // try
                                timeout_ms ); // timeout
  Asynq_Unlock();
  return result;
}

//
// Pop
//

unsigned int
Asynq_Pop( asynq *self, void **pbuf)
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_pop_unlocked(self, pbuf, FALSE, INFINITE );
  Asynq_Unlock();
  return result;
}

unsigned int
Asynq_Pop_Try( asynq *self, void **pbuf )
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_pop_unlocked(self, pbuf, 
                                TRUE,       // try
                                INFINITE ); // timeout
  Asynq_Unlock();
  return result;
}

unsigned int
Asynq_Pop_Timed( asynq *self, void **pbuf, DWORD timeout_ms )
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_pop_unlocked(self, pbuf,
                                FALSE,        // try
                                timeout_ms ); // timeout
  Asynq_Unlock();
  return result;
}

//
// Peek
//

unsigned int
Asynq_Peek( asynq *self, void *buf)
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_peek_unlocked(self, buf, FALSE, INFINITE );
  Asynq_Unlock();
  return result;
}

unsigned int
Asynq_Peek_Try( asynq *self, void *buf )
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_peek_unlocked(self, buf, 
                                TRUE,       // try
                                INFINITE ); // timeout
  Asynq_Unlock();
  return result;
}

unsigned int
Asynq_Peek_Timed( asynq *self, void *buf, DWORD timeout_ms )
{ unsigned int result;
  Asynq_Lock();
  result = _asynq_peek_unlocked(self, buf,
                                FALSE,        // try
                                timeout_ms ); // timeout
  Asynq_Unlock();
  return result;
}

//
// Token buffer management
//
void*  Asynq_Alloc_Token_Buffer( asynq *self )
{ return Guarded_Malloc( self->q->buffer_size_bytes, "Asynq_Alloc_Token_Buffer" );
}

void   Asynq_Free_Token_Buffer ( void *buf   )
{ free(buf);
}