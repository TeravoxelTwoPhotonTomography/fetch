#pragma once

#include "stdafx.h"
#include "ring-fifo.h"


typedef void* PVOID;
TYPE_VECTOR_DECLARE(PVOID);

typedef struct _asynq
{ RingFIFO *q;  

  CRITICAL_SECTION   lock;   // mutex
  HANDLE             notify; // triggered when data becomes available
} asynq;

asynq *Asynq_Alloc   ( size_t buffer_count, size_t buffer_size_bytes );
asynq *Asynq_Inc_Ref ( asynq *self );
void   Asynq_Dec_Ref ( asynq *self );
void   Asynq_Pack    ( asynq *self );

int    Asynq_Swap    ( asynq *self, void **pbuf, size_t index, DWORD timeout_ms );
int    Asynq_Pop     ( asynq *self, void **pbuf, DWORD timeout_ms );
int    Asynq_Push    ( asynq *self, void **pbuf, DWORD timeout_ms, int is_resizable );

int    Asynq_Lock    ( asynq *self, DWORD timeout_ms );
void   Asynq_UnLock  ( asynq *self );

void  *Asynq_Alloc_Token_Buffer( asynq *self );
