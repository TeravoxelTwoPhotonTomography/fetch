#include "stdafx.h"
#include "asynq.h"

//
// Benchmarks
// ----------
// 2009-11-18   1.89 Mops/sec.    ngc   TestAsynQ simple_pushpop no expansion
//                                      Measured for 1e7 ops.
//
//   Notes
//   -----
//   - An "op" (as in ops/sec.) is a single push/pop cycle.
//   - The push/pop time should be independant of the cargo size.
//     It should also be nearly independant of queue size.
//
asynq*
Asynq_Alloc(size_t buffer_count, size_t buffer_size_bytes )
{ asynq *self = (asynq*)Guarded_Malloc( sizeof(asynq), "Asynq_Alloc" );
  
  self->q = RingFIFO_Alloc( buffer_count, buffer_size_bytes );
  
  self->ref_count         = 1;
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
                                    NULL )  // unnamed
  );
  Guarded_Assert_WinErr(
    self->notify_space = CreateEvent( NULL,   // default security attributes
                                      TRUE,   // true ==> manual reset
                                      TRUE,   // initial state is triggered
                                      NULL )  // unnamed
  );  

  return self;
}

asynq* 
Asynq_Ref( asynq *self )
{ return_val_if_fail( self, NULL );  
  return_val_if_fail( self->ref_count > 0, NULL );
  
  Interlocked_Inc_u32( (LONG*) &self->ref_count );
  return self;
}

// Returns 1 on free and 0 otherwise
int 
Asynq_Unref( asynq *self )
{ return_val_if_fail( self, 0 );
  return_val_if_fail( self->ref_count > 0, 0 );
  
  if( Interlocked_Dec_And_Test_u32( (LONG*) &self->ref_count ) ) // Free when last reference is released
  { return_val_if_fail( self->waiting_producers == 0, 0 );       // Gaurantee no one is waiting
    return_val_if_fail( self->waiting_consumers == 0, 0 );
    DeleteCriticalSection( &self->lock );
    if( !CloseHandle( self->notify_data  )) ReportLastWindowsError();
    if( !CloseHandle( self->notify_space )) ReportLastWindowsError();
    if( self->q )
      RingFIFO_Free( self->q );
    self->q = NULL;
	  free(self);
	  return 1;
	}
	return 0;
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
 * Return of 0 indicates a timeout or an 
 *   abandoned wait.
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

static inline unsigned
_asynq_wait_for_free_space( asynq *self, DWORD timeout_ms, const char* msg )
{ HANDLE notify = self->notify_space;
  DWORD res;
  
  Guarded_Assert_WinErr( ResetEvent( notify ) );
  self->waiting_producers++;    
  Asynq_Unlock(self);
  res = WaitForSingleObject( notify, timeout_ms );
  Asynq_Lock(self);    
  self->waiting_producers--;
  
  return _handle_wait_for_result(res,msg);
}

static inline unsigned
_asynq_wait_for_free_data( asynq *self, DWORD timeout_ms, const char* msg )
{ HANDLE notify = self->notify_data;
  DWORD res;
  
  Guarded_Assert_WinErr( ResetEvent( notify ) );
  self->waiting_consumers++;    
  Asynq_Unlock(self);
  res = WaitForSingleObject( notify, timeout_ms );
  Asynq_Lock(self);    
  self->waiting_consumers--;
  
  return _handle_wait_for_result(res,msg);
}

static unsigned int                          // Pushes to head.
_asynq_push_unlocked(                        // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void **pbuf,           // Pointer to the input buffer.  On return from a success, *pbuf points to different but valid buffer for writing.
                      int expand_on_full,    // Allocate additional space in the queue if the queue is full, and then push.
                      int is_try,               // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ RingFIFO* q = self->q;
  if( RingFIFO_Is_Full( q ) && (!expand_on_full || self->waiting_consumers>0 ))
  { if(is_try) return 0;
    if( !_asynq_wait_for_free_space(self, timeout_ms, "Asynq Push") )
      if( RingFIFO_Is_Full( q ) )
        return 0;    
  }
  RingFIFO_Push( q, pbuf, expand_on_full );
  if( self->waiting_consumers )
    SetEvent( self->notify_data );
  return 1;
}

static unsigned int                          // Pops tail buffer.
_asynq_pop_unlocked(                         // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void **pbuf,           // Pointer to the input buffer.  On return from a success, *pbuf points to different but valid buffer for writing.
                      int is_try,               // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ RingFIFO* q = self->q;
  if( RingFIFO_Is_Empty( q ))
  { if(is_try) return 0;
    if( !_asynq_wait_for_free_data(self, timeout_ms, "Asynq Pop") )
      if( RingFIFO_Is_Empty( q ) )
        return 0;
  }
  Guarded_Assert( 0 == RingFIFO_Pop( q, pbuf ) );
  if( self->waiting_producers )
    SetEvent( self->notify_space );
  return 1;
}

static unsigned int                          // Copies data from tail.
_asynq_peek_unlocked(                        // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void *buf,             // Destination buffer.
                      int is_try,               // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ RingFIFO* q = self->q;
  if( RingFIFO_Is_Empty( q ))
  { if(is_try) return 0;
    if( !_asynq_wait_for_free_data(self, timeout_ms, "Asynq Peek") )          
      if( RingFIFO_Is_Empty( q ) )
        return 0;
  }
  RingFIFO_Peek( q, buf );
  return 1;
}

//
// Push
//

unsigned int
Asynq_Push( asynq *self, void **pbuf, int expand_on_full )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_push_unlocked(self, pbuf, expand_on_full, FALSE, INFINITE );
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Push_Try( asynq *self, void **pbuf )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_push_unlocked(self, pbuf, 
                                FALSE,      // expand on full
                                TRUE,       // try
                                INFINITE ); // timeout
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Push_Timed( asynq *self, void **pbuf, DWORD timeout_ms )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_push_unlocked(self, pbuf, 
                                FALSE,        // expand on full
                                FALSE,        // try
                                timeout_ms ); // timeout
  Asynq_Unlock(self);
  return result;
}

//
// Pop
//

unsigned int
Asynq_Pop( asynq *self, void **pbuf)
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_pop_unlocked(self, pbuf, FALSE, INFINITE );
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Pop_Try( asynq *self, void **pbuf )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_pop_unlocked(self, pbuf, 
                                TRUE,       // try
                                INFINITE ); // timeout
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Pop_Timed( asynq *self, void **pbuf, DWORD timeout_ms )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_pop_unlocked(self, pbuf,
                                FALSE,        // try
                                timeout_ms ); // timeout
  Asynq_Unlock(self);
  return result;
}

//
// Peek
//

unsigned int
Asynq_Peek( asynq *self, void *buf)
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_peek_unlocked(self, buf, FALSE, INFINITE );
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Peek_Try( asynq *self, void *buf )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_peek_unlocked(self, buf, 
                                TRUE,       // try
                                INFINITE ); // timeout
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Peek_Timed( asynq *self, void *buf, DWORD timeout_ms )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_peek_unlocked(self, buf,
                                FALSE,        // try
                                timeout_ms ); // timeout
  Asynq_Unlock(self);
  return result;
}

//
// State queries
//

int 
Asynq_Is_Full( asynq *self )
{ int ret;
  Asynq_Lock(self);
  ret = RingFIFO_Is_Full(self->q);
  Asynq_Unlock(self);
  return ret;
}

int Asynq_Is_Empty( asynq *self )
{ int ret;
  Asynq_Lock(self);
  ret = RingFIFO_Is_Empty(self->q);
  Asynq_Unlock(self);
  return ret;
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