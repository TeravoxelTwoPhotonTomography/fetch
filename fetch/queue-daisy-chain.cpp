#include "stdafx.h"
#include "queue-daisy-chain.h"

DCQueue*
DCQueue_Alloc(size_t buffer_count, size_t buffer_size_bytes )
{ DCQueue *self = Guarded_Malloc( sizeof(DCQueue), "DCQueue_Alloc" );
  
  Guarded_Assert( IS_POW2( buffer_count ) );

  self->head = 0;
  self->tail = 0;
  self->buffer_size_bytes = buffer_size_bytes;

  self->ring = vector_PVOID_alloc( buffer_count );
  { vector_PVOID *r = self->ring;
    PVOID *cur = r->contents + r->nelem,
          *beg = r->contents;
    while( cur-- > beg )
      *cur = Guarded_Malloc( buffer_size_bytes, "DCQueue_Alloc: Allocating buffers" );
  }

  return self;
}

void 
DCQueue_Free( DCQueue *self )
{ return_if_fail(self);
  if( self->ring )
  { vector_PVOID *r = self->ring;
    PVOID *cur = r->contents + r->nelem,
          *beg = r->contents;
    while( cur-- > beg )
      free(*cur);
    vector_PVOID_free( r );
    self->ring = NULL;    
  }
  free(self);	
}

void 
DCQueue_Expand( DCQueue *self ) 
{ vector_PVOID *r = self->ring;
  size_t old  = r->nelem,
         head = self->head,
         tail = self->tail,
         buffer_size_bytes = self->buffer_size_bytes;
  int n;

  vector_PVOID_request_pow2( r, old+1 ); // size to next pow2
  self->ring = r;     // update
  n = r->nelem - old; // the number of spots added
  
  Guarded_Assert(n); // I think this should always be >0
  { PVOID *buf = r->contents,
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
                             "DCQueue_Expand: Allocating new buffers" );
  }
}

static inline void     
_swap( DCQueue *self, void **pbuf, size_t idx)
{ vector_PVOID *r = self->ring; 
  idx = MOD_UNSIGNED_POW2( idx, r->nelem );   
  { void **cur = r->contents + idx,
          *t   = *cur;
    *cur  = *pbuf;
    *pbuf = *t;
  }
}

inline void     
DCQueue_Swap_Head( DCQueue *self, void **pbuf)
{ _swap( self, pbuf, self->head );
}

inline void     
DCQueue_Swap_Tail( DCQueue *self, void **pbuf)
{ _swap( self, pbuf, self->tail );
}

unsigned int
DCQueue_Pop( DCQueue *self, void **pbuf)
{ return_val_if( DCQueue_Is_Empty(self), 0);
  DCQueue_Swap_Tail(self,pbuf);
  self->tail++;
  return 1;
}

unsigned int
DCQueue_Push( DCQueue *self, void **pbuf, int expand_on_full)
{ if( DCQueue_Is_Full(self) )
  { if( expand_on_full ) 
    { DCQueue_Expand(self);
      Guarded_Assert( !DCQueue_Is_Empty(self) ); // FIXME: Once this is tested it can be removed
    }
    else
      return 0;
  }
  DCQueue_Swap_Head(self,pbuf);
  self->head++;
  return 1;
}

void*
DCQueue_Alloc_Token_Buffer( DCQueue *self )
{ return Guarded_Malloc( self->buffer_size_bytes, 
                         "DCQueue_Alloc_Token_Buffer" );
}