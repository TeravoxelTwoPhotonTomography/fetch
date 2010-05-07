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
#pragma once

#include "stdafx.h"
#include "ring-fifo.h"

/*
 AsynQ
 -----

 This is a thread-safe asynchronous queue.  It can be used from multiple threads
 without explicit locking.

 The implementation wraps a FIFO queue implemented on a circular store of
 pointers that address distinct, pre-allocated buffers.  

 There are three basic operations: pop, push and peek. Push and pop use a swap
 operation that exchanges the pointer to a token buffer with the pointer to the
 head/tail.  On a push, the input buffer has been filled with data to enqueue.
 On a pop, the input buffer is just a place holder; the contained data may be
 deleted or overwritten.

 The only operation that initiates a blocking copy of an enqueued buffer is
 peek.  Peek copies data from the tail buffer to an input buffer.
 
 Peeking and popping block when the queue is empty.  On termination, if no data
 is being pushed on, waiting consumers will become deadlocked.  There are two
 mechanisms provided for dealing with this:

   1.  Flush waiting consumers by triggering the notify_data event until there
       are no more waiting consumers.  Pops and peeks will return indicating
       failure because they double check the queue state after they're notified.

   2.  Use *_Timed functions. These will indicate failure via the return value.
   
 The one to use depends on the situation.  Flushing all waiting consumers is
 really designed to be performed on shutdown after waiting for threads to
 terminate (with a timeout).  The flush will ensure that consumer threads can
 unblock and exit normally, giving them a chance to release resources.

 Notes to self:
 ..............
 - This seems very similar in intention to the SwapChain in DirectX10.

 Return values:
 -------------
 Where applicable:
  1 indicates Success
  0 indicates Failure
 
*/

#include "stdafx.h"
#include "ring-fifo.h"

typedef struct _asynq
{ RingFIFO *q;  

  u32 ref_count;
  u32 waiting_producers;
  u32 waiting_consumers;
  
  CRITICAL_SECTION   _lock;           // mutex
  HANDLE             notify_space;    // triggered when queue is not full (has available space)
  HANDLE             notify_data;     // triggered when queue is not empty (has available data).
  HANDLE             notify_abort;    // triggered to abort any pending operations (e.g. for shutdown).
} asynq;

asynq *Asynq_Alloc   ( size_t buffer_count, size_t buffer_size_bytes );
asynq *Asynq_Ref     ( asynq *self );
int    Asynq_Unref   ( asynq *self );

unsigned int Asynq_Push       ( asynq *self, void **pbuf, size_t sz, int expand_on_full ); // on overflow, may overwrite or expand
unsigned int Asynq_Push_Copy  ( asynq *self, void  *buf,  size_t sz, int expand_on_full ); // on overflow, same as push
unsigned int Asynq_Push_Try   ( asynq *self, void **pbuf, size_t sz );                     // on overflow, fails immediatly
unsigned int Asynq_Push_Timed ( asynq *self, void **pbuf, size_t sz, DWORD timeout_ms );   // on overflow, waits.  Fails after timeout.

unsigned int Asynq_Pop          ( asynq *self, void **pbuf, size_t sz );                   // on underflow, waits forever.
unsigned int Asynq_Pop_Try      ( asynq *self, void **pbuf, size_t sz );
unsigned int Asynq_Pop_Copy_Try ( asynq *self, void  *buf , size_t sz );
unsigned int Asynq_Pop_Timed    ( asynq *self, void **pbuf, size_t sz, DWORD timeout_ms );

unsigned int Asynq_Peek       ( asynq *self, void **pbuf, size_t sz );
unsigned int Asynq_Peek_Try   ( asynq *self, void **pbuf, size_t sz );
unsigned int Asynq_Peek_Timed ( asynq *self, void **pbuf, size_t sz, DWORD timeout_ms );

       void  Asynq_Flush_Waiting_Consumers( asynq *self );

inline void  Asynq_Lock    ( asynq *self );
inline void  Asynq_Unlock  ( asynq *self );

int Asynq_Is_Full( asynq *self );
int Asynq_Is_Empty( asynq *self );

inline void   Asynq_Resize_Buffers(asynq* self, size_t nbytes);                            // when nbytes is less than current size, does nothing

void*  Asynq_Token_Buffer_Alloc( asynq *self );
void*  Asynq_Token_Buffer_Alloc_And_Copy ( asynq *self, void *src );
void   Asynq_Token_Buffer_Free ( void *buf );
