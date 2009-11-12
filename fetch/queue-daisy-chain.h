#pragma once

#include "stdafx.h"

typedef void* PVOID;
TYPE_VECTOR_DECLARE(PVOID);

typedef struct _queue_daisy_chain
{ vector_PVOID *ring;
  size_t        head; // write cursor
  size_t        tail; // read  cursor

  size_t        buffer_size_bytes;
} DCQueue;

DCQueue*    DCQueue_Alloc   ( size_t buffer_count, size_t buffer_size_bytes );
void        DCQueue_Expand  ( DCQueue *self );
void        DCQueue_Free    ( DCQueue *self );

inline void  DCQueue_Swap_Head ( DCQueue *self, void **pbuf);
inline void  DCQueue_Swap_Tail ( DCQueue *self, void **pbuf);
unsigned int DCQueue_Pop       ( DCQueue *self, void **pbuf);
unsigned int DCQueue_Push      ( DCQueue *self, void **pbuf, int expand_on_full);

void*       DCQueue_Alloc_Token_Buffer( DCQueue *self );

#define     DCQueue_Is_Empty(self) ( (self)->head == (self)->tail )
#define     DCQueue_Is_Full (self) ( (self)->head == (self)->tail + (self)->ring->nelem )
