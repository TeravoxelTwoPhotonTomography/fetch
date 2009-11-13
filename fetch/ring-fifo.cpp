#include "stdafx.h"
#include "ring-fifo.h"

RingFIFO*
RingFIFO_Alloc(size_t buffer_count, size_t buffer_size_bytes )
{ RingFIFO *self = Guarded_Malloc( sizeof(RingFIFO), "RingFIFO_Alloc" );
  
  Guarded_Assert( IS_POW2( buffer_count ) );

  self->head = 0;
  self->tail = 0;
  self->buffer_size_bytes = buffer_size_bytes;

  self->ring = vector_PVOID_alloc( buffer_count );
  { vector_PVOID *r = self->ring;
    PVOID *cur = r->contents + r->nelem,
          *beg = r->contents;
    while( cur-- > beg )
      *cur = Guarded_Malloc( buffer_size_bytes, "RingFIFO_Alloc: Allocating buffers" );
  }

  return self;
}

void 
RingFIFO_Free( RingFIFO *self )
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
RingFIFO_Expand( RingFIFO *self ) 
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
                             "RingFIFO_Expand: Allocating new buffers" );
  }
}

static inline void     
_swap( RingFIFO *self, void **pbuf, size_t idx)
{ vector_PVOID *r = self->ring; 
  idx = MOD_UNSIGNED_POW2( idx, r->nelem );   
  { void **cur = r->contents + idx,
          *t   = *cur;
    *cur  = *pbuf;
    *pbuf = *t;
  }
}

inline void     
RingFIFO_Swap_Head( RingFIFO *self, void **pbuf)
{ _swap( self, pbuf, self->head );
}

inline void     
RingFIFO_Swap_Tail( RingFIFO *self, void **pbuf)
{ _swap( self, pbuf, self->tail );
}

unsigned int
RingFIFO_Pop( RingFIFO *self, void **pbuf)
{ return_val_if( RingFIFO_Is_Empty(self), 0);
  RingFIFO_Swap_Tail(self,pbuf);
  self->tail++;
  return 1;
}

unsigned int
RingFIFO_Push( RingFIFO *self, void **pbuf, int expand_on_full)
{ if( RingFIFO_Is_Full(self) )
  { if( expand_on_full ) 
    { RingFIFO_Expand(self);
      Guarded_Assert( !RingFIFO_Is_Empty(self) ); // FIXME: Once this is tested it can be removed
    }
    else
      return 0;
  }
  RingFIFO_Swap_Head(self,pbuf);
  self->head++;
  return 1;
}

void*
RingFIFO_Alloc_Token_Buffer( RingFIFO *self )
{ return Guarded_Malloc( self->buffer_size_bytes, 
                         "RingFIFO_Alloc_Token_Buffer" );
}