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

  Guarded_Assert( RingFIFO_Is_Empty(self) ); // FIXME: Remove when done testing
  Guarded_Assert(!RingFIFO_Is_Full (self) );
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
         n,
         head = MOD_UNSIGNED_POW2( self->head, old ),
         tail = MOD_UNSIGNED_POW2( self->tail, old ),
         buffer_size_bytes = self->buffer_size_bytes;

  vector_PVOID_request_pow2( r, old+1 ); // size to next pow2  
  n = r->nelem - old; // the number of slots added
    
  { PVOID *buf = r->contents,
          *beg, *cur;
    
    // Need to gaurantee that the new interval is outside of the 
    //    head -> tail interval (active data runs from tail to head).
    if( head > tail ) 
    { // Copy right half to put gap between write point and read point
      beg = buf+head;
      cur = buf+head+n;
      self->head += n;  // Note: self->head has not been modded, where head has.
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

static inline size_t
_swap( RingFIFO *self, void **pbuf, size_t idx)
{ vector_PVOID *r = self->ring;                  // taking the mod on read (here) saves some ops                     
  idx = MOD_UNSIGNED_POW2( idx, r->nelem );      //   relative to on write since that involve writing to a reference.
  { void **cur = r->contents + idx,              // in pop/push idx could overflow, but that 
          *t   = *cur;                           //   would be ok since r->nelem is a divisor of
    *cur  = *pbuf;                               //   2^sizeof(size_t)
    *pbuf = *t;                                  // Note that the ...Is_Empty and ...Is_Full 
  }                                              //   constructs still work with the possibility of
  return idx;                                    //   an overflow
}

inline unsigned int
RingFIFO_Pop( RingFIFO *self, void **pbuf)
{ return_val_if( RingFIFO_Is_Empty(self), 1);
  _swap( self, pbuf, self->tail++ );
  return 0;
}

inline unsigned int
RingFIFO_Peek( RingFIFO *self, void *buf)
{ return_val_if( RingFIFO_Is_Empty(self), 1);
  { vector_PVOID *r = self->ring;
    memcpy( buf, 
            r->contents[MOD_UNSIGNED_POW2(self->tail, r->nelem)],
            self->buffer_size_bytes );
  }
  return 0;
}

inline unsigned int
RingFIFO_Push_Try( RingFIFO *self, void **pbuf)
{ if( RingFIFO_Is_Full(self) )
    return 1;
  _swap( self, pbuf, self->head++ );
  return 0;
}

unsigned int
RingFIFO_Push( RingFIFO *self, void **pbuf, int expand_on_full)
{ return_val_if( RingFIFO_Push_Try(self, pbuf)==0, 0 );
  // Handle when full
  if( expand_on_full )      // Expand
  { RingFIFO_Expand(self);
    Guarded_Assert( !RingFIFO_Is_Full(self) );  // FIXME: Once this is tested it can be removed
    Guarded_Assert( !RingFIFO_Is_Empty(self) ); // FIXME: Once this is tested it can be removed
    return 0;
  } 
  // Overwrite
  self->tail++;
  _swap( self, pbuf, self->head++ );
  return 1;
}

void*
RingFIFO_Alloc_Token_Buffer( RingFIFO *self )
{ return Guarded_Malloc( self->buffer_size_bytes, 
                         "RingFIFO_Alloc_Token_Buffer" );
}
