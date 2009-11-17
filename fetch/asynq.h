#pragma once

#include "stdafx.h"
#include "ring-fifo.h"

/*
 AsynQ
 -----

 This is a threadsafe asynchronous queue.  It can be used from multiple threads
 without explicit locking.

 The implimentation wraps a FIFO queue implimented on a circular store of 
 pointers that address distinct, pre-allocated buffers.  

 There are three basic operations: pop, push and peek. Push and pop use a swap 
 operation that exchanges the pointer to a token buffer with the pointer to the
 head/tail.  On a push, the input buffer has been filled with data to enqueue.
 On a pop, the input buffer is just a place holder; the contained data may be 
 deleted or overwritten.

 The only operation that initiates a blocking copy of an enqueued buffer is peek.
 Peek copies data from the tail buffer to an input buffer.
 
 Notes to self:
 ..............
 - on free, need to wait till no more waiting threads
   I think this will require reference counting
 - need to gaurantee that all pushed data gets popped, if possible
 - This seems very similar in intention to the SwapChain in DirectX10.
 
*/

#include "stdafx.h"
#include "ring-fifo.h"

typedef struct _asynq
{ RingFIFO *q;  

  u32 ref_count;
  u32 waiting_producers;
  u32 waiting_consumers;
  
  CRITICAL_SECTION   lock;         // mutex
  HANDLE             notify_space; // triggered when queue is not full (has available space)
  HANDLE             notify_data;  // triggered when queue is not empty (has available data).
} asynq;

asynq *Asynq_Alloc   ( size_t buffer_count, size_t buffer_size_bytes );
asynq *Asynq_Ref     ( asynq *self );
void   Asynq_Unref   ( asynq *self );

unsigned int Asynq_Push       ( asynq *self, void **pbuf, int expand_on_full );
unsigned int Asynq_Push_Try   ( asynq *self, void **pbuf );
unsigned int Asynq_Push_Timed ( asynq *self, void **pbuf, DWORD timeout_ms );

unsigned int Asynq_Pop        ( asynq *self, void **pbuf );
unsigned int Asynq_Pop_Try    ( asynq *self, void **pbuf );
unsigned int Asynq_Pop_Timed  ( asynq *self, void **pbuf, DWORD timeout_ms );

unsigned int Asynq_Peek       ( asynq *self, void  *buf );
unsigned int Asynq_Peek_Try   ( asynq *self, void  *buf );
unsigned int Asynq_Peek_Timed ( asynq *self, void  *buf, DWORD timeout_ms );

inline void  Asynq_Lock    ( asynq *self );
inline void  Asynq_Unlock  ( asynq *self );

void*  Asynq_Alloc_Token_Buffer( asynq *self );
void   Asynq_Free_Token_Buffer ( void *buf   );
