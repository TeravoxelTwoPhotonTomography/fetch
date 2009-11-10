#pragma once

#include "stdafx.h"

typedef void* PVOID;
TYPE_VECTOR_DECLARE(PVOID);

typedef struct _asynq
{ vector_PVOID ring;
  size_t       head; // write cursor
  size_t       tail; // read  cursor

  size_t       buffer_size_bytes;

  HANDLE       lock;
  HANDLE       event_destroy;
  HANDLE       event_pop;
  HANDLE       event_push;
} asynq;

asynq *Asynq_Alloc   ( size_t buffer_count, size_t buffer_size_bytes );
void   Asynq_Expand  ( asynq *self );
void   Asynq_Pack    ( asynq *self );
void   Asynq_Free    ( asynq *self );

int    Asynq_Swap    ( asynq *self, void **pbuf, size_t index, DWORD timeout_ms );
int    Asynq_Pop     ( asynq *self, void **pbuf, DWORD timeout_ms );
int    Asynq_Push    ( asynq *self, void **pbuf, DWORD timeout_ms, int is_resizable );

int    Asynq_Lock    ( asynq *self, DWORD timeout_ms );
void   Asynq_UnLock  ( asynq *self );

void  *Asynq_Alloc_Token_Buffer( asynq *self );
