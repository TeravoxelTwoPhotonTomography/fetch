/*
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 26, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "stdafx.h"
#include "asynq.h"


#if 1
#define DEBUG_ASYNQ_HANDLE_WAIT_FOR_RESULT
#define DEBUG_ASYNQ_FLUSH_WAITING_CONSUMERS
#define DEBUG_ASYNQ_UNREF

#endif

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
    InitializeCriticalSectionAndSpinCount( &self->_lock, 
                                            0x80000400 ) 
  );
  
  { HANDLE  *es[3] = { &self->notify_data,
                       &self->notify_space,
                       &self->notify_abort};
    BOOL states[3] = { FALSE,
                       TRUE,
                       FALSE };
    int i = 3;
    while(i--)
      Guarded_Assert_WinErr(
        *es[i] = CreateEvent(NULL,       // default security attributes
                             TRUE,       // true ==> manual reset
                             states[i],  // initial state
                             NULL ));    // unnamed
  }

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
// That is, returns 1 if ref count reached 0.
int 
Asynq_Unref( asynq *self )
{ return_val_if_fail( self, 0 );
  return_val_if_fail( self->ref_count > 0, 0 );
  
  if( Interlocked_Dec_And_Test_u32( (LONG*) &self->ref_count ) ) // Free when last reference is released
  { if(self->waiting_consumers>0 || self->waiting_producers>0)
    { SetEvent( self->notify_abort );
      return 0;
    }  
    DeleteCriticalSection( &self->_lock );
    if( !CloseHandle( self->notify_data  )) ReportLastWindowsError();
    if( !CloseHandle( self->notify_space )) ReportLastWindowsError();
    if( !CloseHandle( self->notify_abort )) ReportLastWindowsError();
    if( self->q )
      RingFIFO_Free( self->q );
    self->q = NULL;
#ifdef DEBUG_ASYNQ_UNREF
    debug("Free Asynq at 0x%p\r\n",self);
#endif	  
	  free(self);
	  return 1;
	}
	return 0;
}

inline void 
Asynq_Lock ( asynq *self )
{ EnterCriticalSection( &self->_lock );
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
#ifdef DEBUG_ASYNQ_HANDLE_WAIT_FOR_RESULT  
  if( result == WAIT_ABANDONED )
    warning("Asynq: Wait abandoned\r\n\t%s\r\n",msg);
  if( result == WAIT_TIMEOUT )
    warning("Asynq: Wait timeout\r\n\t%s\r\n",msg);  
#endif
  return 0;
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
_handle_wait_for_multiple_result(DWORD result, int n, const char* msg)
{ return_val_if( result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0+n, 1 );
  Guarded_Assert_WinErr( result != WAIT_FAILED );
#ifdef DEBUG_ASYNQ_HANDLE_WAIT_FOR_RESULT  
  if( result >= WAIT_ABANDONED_0 && result < WAIT_ABANDONED_0+n )
    warning("Asynq: Wait abandoned on object %d\r\n"
            "\t%s\r\n",result - WAIT_ABANDONED_0,msg);
  if( result == WAIT_TIMEOUT )
    warning("Asynq: Wait timeout\r\n"
            "\t%s\r\n",msg);  
#endif
  return 0;
}

inline void 
Asynq_Unlock ( asynq *self )
{ LeaveCriticalSection( &self->_lock );
}

//
// Fundamental pop/push behavior
//

static inline unsigned
_asynq_wait_for_space( asynq *self, DWORD timeout_ms, const char* msg )
{ HANDLE notify[2] = {self->notify_space, self->notify_abort};
  DWORD res;
  
  Guarded_Assert_WinErr( ResetEvent( notify[0] ) );
  self->waiting_producers++;    
  Asynq_Unlock(self);
  res = WaitForMultipleObjects(2, notify, FALSE /*any*/, timeout_ms );
  Asynq_Lock(self);    
  self->waiting_producers--;
  
  return _handle_wait_for_multiple_result(res,2,msg);
}

static inline unsigned
_asynq_wait_for_data( asynq *self, DWORD timeout_ms, const char* msg )
{ HANDLE notify[2] = {self->notify_data, self->notify_abort};
  DWORD res;
  
  Guarded_Assert_WinErr( ResetEvent( notify[0] ) );
  self->waiting_consumers++;    
  Asynq_Unlock(self);
  res = WaitForMultipleObjects(2, notify, FALSE/*any*/,timeout_ms );
  Asynq_Lock(self);    
  self->waiting_consumers--;
  
  return _handle_wait_for_multiple_result(res,2,msg);
}

static unsigned int                          // Pushes to head.
_asynq_push_unlocked(                        // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void **pbuf,           // Pointer to the input buffer.  On return from a success, *pbuf points to different but valid buffer for writing.
                      size_t sz,             // The size of the input buffer (bytes).
                      int expand_on_full,    // Allocate additional space in the queue if the queue is full, and then push.
                      int block_on_full,     // Block (wait) when the queue is full rather than overwrite data.
                      int is_try,               // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ RingFIFO* q = self->q;
  if( RingFIFO_Is_Full( q ))
  { if(is_try) return 0;
    if( block_on_full || self->waiting_consumers>0 )
      if( !_asynq_wait_for_space(self, timeout_ms, "Asynq Push") )
        if( RingFIFO_Is_Full( q ) )
          return 0;    
  }
  RingFIFO_Push( q, pbuf, sz, expand_on_full );
  if( self->waiting_consumers )
    SetEvent( self->notify_data );
  return 1;
}

static unsigned int                          // Pops tail buffer.
_asynq_pop_unlocked(                         // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void **pbuf,           // Pointer to the input buffer.  On return from a success, *pbuf points to different but valid buffer for writing.
                      size_t sz,             // The size of the input buffer (bytes).
                      int is_try,            // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ RingFIFO* q = self->q;
  if( RingFIFO_Is_Empty( q ))
  { if(is_try) return 0;
    if( !_asynq_wait_for_data(self, timeout_ms, "Asynq Pop") )
      if( RingFIFO_Is_Empty( q ) )
        return 0;
  }
  return_val_if( RingFIFO_Pop( q, pbuf, sz ), 0 ); // Fail if the queue is empty
  if( self->waiting_producers )
    SetEvent( self->notify_space );
  return 1;
}

static unsigned int                          // Copies data from tail.
_asynq_peek_unlocked(                        // Returns 1 on success, 0 otherwise.
                      asynq *self,           // 
                      void **buf,            // Destination buffer.
                      size_t sz,             // size of *buf in bytes
                      int is_try,            // if true, push returns immediately whether successful or not.  Never waits.
                      DWORD timeout_ms )     // Time to wait for a self->notify_space event.  Set to INFINITE for no timeout.
{ RingFIFO* q = self->q;
  if( RingFIFO_Is_Empty( q ))
  { if(is_try) return 0;
    if( !_asynq_wait_for_data(self, timeout_ms, "Asynq Peek") )          
      if( RingFIFO_Is_Empty( q ) )
        return 0;
  }
  return 0== RingFIFO_Peek( q, buf, sz );  
}

//
// Push
//

unsigned int
Asynq_Push( asynq *self, void **pbuf, size_t sz, int expand_on_full )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_push_unlocked(self, pbuf, sz, expand_on_full, FALSE, FALSE, INFINITE );
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Push_Copy(asynq *self, void *buf, size_t sz, int expand_on_full )
{ unsigned int result;
  static void *token = 0;
  Asynq_Lock(self);
  if( !token ) // one-time-initialize working space
    token = Asynq_Token_Buffer_Alloc(self);
  memcpy(token,buf, self->q->buffer_size_bytes);
  result = _asynq_push_unlocked(self, &token, sz, expand_on_full, FALSE, FALSE, INFINITE );
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Push_Try( asynq *self, void **pbuf, size_t sz)
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_push_unlocked(self, pbuf, sz,
                                FALSE,      // expand on full
                                FALSE,      // block on full
                                TRUE,       // try
                                INFINITE ); // timeout
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Push_Timed( asynq *self, void **pbuf, size_t sz, DWORD timeout_ms ) // Returns 1 on success, 0 otherwise.
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_push_unlocked(self, pbuf, sz,
                                FALSE,        // expand on full
                                TRUE,         // block on full
                                FALSE,        // try
                                timeout_ms ); // timeout
  Asynq_Unlock(self);
  return result;
}

//
// Pop
//

unsigned int
Asynq_Pop( asynq *self, void **pbuf, size_t sz)
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_pop_unlocked(self, pbuf, sz, FALSE, INFINITE );
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Pop_Try( asynq *self, void **pbuf, size_t sz)
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_pop_unlocked(self, pbuf, sz,
                                TRUE,       // try
                                INFINITE ); // timeout
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Pop_Copy_Try( asynq *self, void *buf, size_t sz)
{ unsigned int result;
  static void* token;
  Asynq_Lock(self);
  if( !token )
    token = Asynq_Token_Buffer_Alloc(self);
  result = _asynq_pop_unlocked(self, &token, sz,
                                TRUE,       // try
                                INFINITE ); // timeout
  memcpy(buf,token,self->q->buffer_size_bytes);
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Pop_Timed( asynq *self, void **pbuf, size_t sz, DWORD timeout_ms )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_pop_unlocked(self, pbuf, sz,
                                FALSE,        // try
                                timeout_ms ); // timeout
  Asynq_Unlock(self);
  return result;
}

//
// Peek
//

unsigned int
Asynq_Peek( asynq *self, void **buf, size_t sz)
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_peek_unlocked(self, buf, sz, FALSE, INFINITE );
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Peek_Try( asynq *self, void **buf, size_t sz )
{ unsigned int result;
  Asynq_Lock(self);
  result = _asynq_peek_unlocked(self, buf, sz, 
                                TRUE,       // try
                                INFINITE ); // timeout
  Asynq_Unlock(self);
  return result;
}

unsigned int
Asynq_Peek_Timed( asynq *self, void **buf, size_t sz, DWORD timeout_ms )
{ unsigned  int result;
  Asynq_Lock(self);
  result = _asynq_peek_unlocked(self, buf, sz,
                                FALSE,        // try
                                timeout_ms ); // timeout
  Asynq_Unlock(self);
  return result;
}

//
// Producer/Consumer management
//

void Asynq_Flush_Waiting_Consumers( asynq *self )
{ SetEvent( self->notify_abort );
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

extern inline void
Asynq_Resize_Buffers(asynq* self, size_t nbytes)
{ Asynq_Lock(self);
  RingFIFO_Resize(self->q,nbytes);
  Asynq_Unlock(self);
}

void*  Asynq_Token_Buffer_Alloc( asynq *self )
{ return Guarded_Malloc( self->q->buffer_size_bytes, "Asynq_Token_Buffer_Alloc" );
}

void*  Asynq_Token_Buffer_Alloc_And_Copy ( asynq *self, void *src )
{ void *dst = Asynq_Token_Buffer_Alloc(self);
  memcpy(dst,src,self->q->buffer_size_bytes);
  return dst;
}

void   Asynq_Token_Buffer_Free ( void *buf )
{ free(buf);
}
