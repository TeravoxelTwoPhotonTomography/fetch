#pragma once

#include "stdafx.h"

typedef void* PVOID;
TYPE_VECTOR_DECLARE(PVOID);

typedef struct _ring_fifo
{ vector_PVOID *ring;
  size_t        head; // write cursor
  size_t        tail; // read  cursor

  size_t        buffer_size_bytes;
} RingFIFO;

RingFIFO*   RingFIFO_Alloc   ( size_t buffer_count, size_t buffer_size_bytes );
void        RingFIFO_Expand  ( RingFIFO *self );
void        RingFIFO_Free    ( RingFIFO *self );

inline void  RingFIFO_Swap_Head ( RingFIFO *self, void **pbuf);
inline void  RingFIFO_Swap_Tail ( RingFIFO *self, void **pbuf);
unsigned int RingFIFO_Pop       ( RingFIFO *self, void **pbuf);
unsigned int RingFIFO_Push      ( RingFIFO *self, void **pbuf, int expand_on_full);

void*       RingFIFO_Alloc_Token_Buffer( RingFIFO *self );

#define     RingFIFO_Is_Empty(self) ( (self)->head == (self)->tail )
#define     RingFIFO_Is_Full (self) ( (self)->head == (self)->tail + (self)->ring->nelem )
