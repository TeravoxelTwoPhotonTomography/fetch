#include "stdafx.h"
#include "device.h"
#include "device-digitizer.h"
#include "frame.h"


//----------------------------------------------------------------------------
//
//  Averager
//

struct _averager_context
{ int ichan;
  int ntimes;
  int Bpp_in;
}

unsigned int
_Averager_Cfg( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ struct _averager_context *ctx = (struct _averager_context*) d->context;
  size_t qsize, bufsize;
  Guarded_Assert( d->task->out == NULL );
  qsize = 16;
  bufsize  = in->contents[ ctx->ichan ]->q->buffer_size_bytes;
  bufsize *= sizeof(float)/(float)Bpp_in;
  DeviceTask_Alloc_Outputs( d->task, 1, &qsize, &bufsize );
  return 0; // success
}

unsigned int
_Averager_Proc( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ struct _averager_context *ctx = (struct _averager_context*) d->context;

  asynq *qsrc = in->contents[ ctx->ichan ],
        *qdst = out->contents[ ctx->ichan ];

  void *buf = Asynq_Token_Buffer_Alloc(qsrc),
       *acc = Asynq_Token_Buffer_Alloc(qdst);
  do
  { while( Asynq_Pop(q, &buf) )
    { // XXX: Here
      // Argh...how to add?
    }
  } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
  Asynq_Token_Buffer_Free(buf);
  Asynq_Token_Buffer_Free(acc);
  return 0;
}
