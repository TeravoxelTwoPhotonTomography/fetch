/** \file
    \ref Chan is a zero-copy multi-producer mutli-consumer asynchronous queue
    for passing messages between threads.

    Reading and writing to the queue are both performed via Chan_Next().
    Whether a "read" or a "write" happens depends on the \a mode passed to
    Chan_Open(), which returns a reference-counted "reader" or "writer"
    reference to the queue.
    
    Note that "opening" a channel is different than "allocating" it.  A \ref
    Chan is typically allocated once, but may be opened and closed many times.
    Chan_Alloc() initializes and reserves memory for the queue.  Internally,
    Chan_Open() does little more than some bookkeeping.
    
    \ref Chan was designed so that Chan_Next() operations block the calling
    thread under some circumstances.  A read will wait for data if as long as a
    writer has been opened for that \ref Chan elsewhere.  When data becomes
    available, the read will return successfully returning a message from the
    queue.  If the last writer is closed (Chan_Close()), the read returns an
    error.  Writes work similarly by blocking until space is available on the
    queue as long as readers are available.

    With proper use, this design garauntees that certain networks of threads
    (namely, directed acyclic graphs) connected by \ref Chan instances will
    deliver every message emited from a producer to a consumer in that network.
    Once the network is assembled, messages will be recieved in topological
    order.

    An example producer thread might look something like this:
    \code
    void producer(Chan *q)
    { void  *data = Chan_Token_Buffer_Alloc(q);
      size_t nbytes;
      Chan  *output = Chan_Open(q,CHAN_WRITE);
      while( data=getdata(&nbytes) )
        if( CHAN_FAILURE( Chan_Next(output,&data,&nbytes)))
          break;
      Chan_Close(output);
      Chan_Token_Buffer_Free(data);
    }
    \endcode

    \author Nathan Clack <clackn@janelia.hhmi.org>
    \date   Feb. 2011
  */
#pragma once
#include <stdlib.h>
#include "config.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef void  Chan;

typedef enum _chan_mode
{ CHAN_NONE=0,
  CHAN_PEEK=0,
  CHAN_READ,
  CHAN_WRITE,
  CHAN_MODE_MAX,
} ChanMode;

       Chan  *Chan_Alloc              ( size_t buffer_count, size_t buffer_size_bytes);
extern Chan  *Chan_Alloc_Copy         ( Chan *chan);
       Chan  *Chan_Open               ( Chan *self, ChanMode mode);                                    // does ref counting and access type
       int    Chan_Close              ( Chan *self );                                                  // does ref counting

extern Chan  *Chan_Id                 ( Chan *self );

unsigned Chan_Get_Ref_Count      ( Chan* self);
void     Chan_Wait_For_Ref_Count ( Chan* self, size_t n);
void     Chan_Wait_For_Writer_Count ( Chan* self,size_t n);
void     Chan_Wait_For_Have_Reader( Chan* self);
void     Chan_Set_Expand_On_Full ( Chan* self, int  expand_on_full);                           // default: no expand

//
// ----
// Next
// ----
//
// Requires a mode to be set.
// For Chan's open in read mode, does a pop.  Reads will fail if there
//     are no writers for the channel and the channel is empty.
//     Otherwise, when the channel is empty, reads will block.
// For chan's open in write mode, does a push.  Writes will block when
//     the channel is full.
//
//   *    Overflow                       Underflow
// =====  ============================   ===================
// -      Waits or expands.              Fails if no sources, otherwise waits.
// Copy   Waits or expands.              Fails if no sources, otherwise waits.
// Try    Fails immediately              Fails immediately.
// Timed  Waits.  Fails after timeout.   Fails immediately if no sources, otherwise waits till timeout.
//

unsigned int Chan_Next         ( Chan *self,  void **pbuf, size_t sz); ///< Pops next item. Returns failure if there are no writers, otherwise waits.
unsigned int Chan_Next_Copy    ( Chan *self,  void  *buf,  size_t sz); ///< Pops a copy of the next item.  Returns failure if there are no writers, otherwise waits.
unsigned int Chan_Next_Try     ( Chan *self,  void **pbuf, size_t sz); ///< If no item's are on the queue, return failure.  Otherwise, pop the next item.
unsigned int Chan_Next_Copy_Try( Chan *self_, void  *buf,  size_t sz); ///< If no item's are on the queue, return failure.  Otherwise, pop a copy of the next item. 
unsigned int Chan_Next_Timed   ( Chan *self,  void **pbuf, size_t sz,   unsigned timeout_ms ); ///< Just like Chan_Next(), but any waiting is limited by the timeout.

// ----
// Peek
// ----
//
// Requires read mode.
//
unsigned int Chan_Peek       ( Chan *self, void **pbuf, size_t sz );
unsigned int Chan_Peek_Try   ( Chan *self, void **pbuf, size_t sz );
unsigned int Chan_Peek_Timed ( Chan *self, void **pbuf, size_t sz, unsigned timeout_ms );

// -----------------
// Memory management
// -----------------
//
// Resize: when nbytes is less than current size, does nothing

int         Chan_Is_Full                    ( Chan *self );
int         Chan_Is_Empty                   ( Chan *self );
extern void Chan_Resize                     ( Chan *self, size_t nbytes);

void*       Chan_Token_Buffer_Alloc         ( Chan *self );
void*       Chan_Token_Buffer_Alloc_And_Copy( Chan *self, void *src );
void        Chan_Token_Buffer_Free          ( void *buf );
extern size_t Chan_Buffer_Size_Bytes        ( Chan *self );
extern size_t Chan_Buffer_Count             ( Chan *self );


#define CHAN_SUCCESS(expr) ((expr)==0)
#define CHAN_FAILURE(expr) (!CHAN_SUCCESS(expr))

#ifdef __cplusplus
}
#endif

