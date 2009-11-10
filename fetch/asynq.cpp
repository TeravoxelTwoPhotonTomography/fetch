#include "stdafx.h"
#include "asynq.h"

int Asynq_Lock( asynq *self, DWORD timeout_ms )
{
}

asynq *Asynq_Alloc(size_t buffer_count, size_t buffer_size_bytes )
{ asynq *self = Guarded_Malloc( sizeof(asynq), "Asynq_Alloc" );
  
  Guarded_Assert( IS_POW2( buffer_count ) );

  self->head = 0;
  self->tail = 0;
  self->buffer_size_bytes = buffer_size_bytes;

  self->ring = vector_PVOID_alloc( buffer_count );
  { PVOID *cur = self->ring.contents + self->ring.nelem,
          *beg = self->ring.contents;
    while( cur-- > beg )
      *cur = Guarded_Malloc( buffer_size_bytes, "Asynq_Alloc: Allocating buffers" );
  }

  // Initialize syncronization primitives
  Guarded_Assert_WinErr(
    self->lock = CreateMutex( NULL,   // default security attributes
                              FALSE,  // initially unowned
                              NULL ); // unnamed
  );
  Guarded_Assert_WinErr(
    self->event_destroy = CreateEvent( NULL,   // default security attributes
                                       TRUE,   // manual reset
                                       FALSE,  // initial state is untriggered
                                       NULL ); // unnamed
  );
  Guarded_Assert_WinErr(
    self->event_pop     = CreateEvent( NULL,   // default security attributes
                                       FALSE,  // manual reset
                                       FALSE,  // initial state is untriggered
                                       NULL ); // unnamed
  );
  Guarded_Assert_WinErr(
    self->event_push    = CreateEvent( NULL,   // default security attributes
                                       FALSE,  // manual reset
                                       FALSE,  // initial state is untriggered
                                       NULL ); // unnamed
  );

  return self;
}

void Asynq_Expand( asynq *self )
{ 
  { size_t old  = self->ring.nelem,
           head = self->head,
           tail = self->tail,
           buffer_size_bytes = self->buffer_size_bytes;
    int n;

    vector_PVOID_request_pow2( &self->ring, old+1 ); // size to next pow2
    n = self->ring.nelem - old; // the number of spots added

    Guarded_Assert(n); // I think this should always be >0
    { PVOID *buf = self->ring.contents,
            *beg, *cur;
      
      // Need to gaurantee that the new interval is outside of the 
      //    head -> tail interval.
      if( head > tail ) 
      { // Copy right half to put gap between write point and read point
        beg = buf+head;
        cur = buf+head+n;
        memmove( cur, beg, old-head );
      } else
      { beg = old;
        cur = old+n;
      }
      while( cur-- > beg )
        *cur = Guarded_Malloc( buffer_size_bytes, 
                               "Asynq_Expand: Allocating new buffers" );
    }
  }
}