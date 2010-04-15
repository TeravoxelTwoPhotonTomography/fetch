#include "stdafx.h"
#include "device-task.h"

TYPE_VECTOR_DEFINE( PASYNQ );

//
// Device_Task_Thread_Proc is the main function of a 
// task's thread.
//
DWORD WINAPI Device_Task_Thread_Proc( LPVOID lpParam )
{ DWORD result;
  Device        *d = (Device*) lpParam;
  DeviceTask *task = d->task;
  result = (task->run_proc)( d, task->in, task->out );
  // Transition back to stop state when run proc returns
  Device_Lock(d);
  d->is_running = 0;  
  CloseHandle(d->thread);
  d->thread = INVALID_HANDLE_VALUE;
  Device_Unlock(d);
  
  return result;  
}

void
DeviceTask_Free_Inputs( DeviceTask* self )
{ if( self->in )
  { size_t n = self->in->count;
    while(n--)
      Asynq_Unref( self->in->contents[n] );
    vector_PASYNQ_free( self->in );
    self->in = NULL;
  }
}

void
DeviceTask_Free_Outputs( DeviceTask* self )
{ if( self->out )
  { size_t n = self->out->count;
    while(n--)
      Asynq_Unref( self->out->contents[n] );
    vector_PASYNQ_free( self->out );
    self->out = NULL;
  }
}

// FIXME: make this more of a resize op to avoid thrashing
void
DeviceTask_Alloc_Inputs( DeviceTask* self,
                             size_t  num_inputs,
                             size_t *input_queue_size, 
                             size_t *input_buffer_size)
{ if( num_inputs )
  { DeviceTask_Free_Inputs( self );
    self->in  = vector_PASYNQ_alloc( num_inputs  ); 
    while( num_inputs-- )
      self->in->contents[num_inputs] 
          = Asynq_Alloc( input_queue_size [num_inputs],
                         input_buffer_size[num_inputs] );
    self->in->count = self->in->nelem; // resizable, so use count rather than nelem alone.
  }                                    //            start full
}

// FIXME: make this more of a resize op to avoid thrashing
void
DeviceTask_Alloc_Outputs( DeviceTask* self,
                              size_t  num_outputs,
                              size_t *output_queue_size, 
                              size_t *output_buffer_size)
{ if( num_outputs )
  { DeviceTask_Free_Outputs( self );
    self->out  = vector_PASYNQ_alloc( num_outputs ); 
    while( num_outputs-- )
      self->out->contents[num_outputs] 
          = Asynq_Alloc( output_queue_size[num_outputs],
                         output_buffer_size[num_outputs] );
    self->out->count = self->out->nelem; // resizable, so use count rather than nelem alone.
  }                                      //            start full
}

DeviceTask*
DeviceTask_Alloc( fp_device_task_cfg_proc  cfg, 
                  fp_device_task_run_proc  run )
{ DeviceTask* self = (DeviceTask*)Guarded_Calloc(1, sizeof(DeviceTask),"DeviceTask_Alloc");
  
  self->in = self->out = NULL;
  self->cfg_proc  = cfg;
  self->run_proc  = run;
  self->main     = &Device_Task_Thread_Proc;
  
  return self;
}

void
DeviceTask_Free( DeviceTask *self )
{ if(self)
  { self->cfg_proc = NULL;
    self->run_proc = NULL;
    DeviceTask_Free_Inputs( self );
    DeviceTask_Free_Outputs( self );
  }
}

// Destination channel inherits the existing channel's properties.
// If both channels exist, the source properties are inhereted.
// One channel must exist.
void
DeviceTask_Connect( DeviceTask *dst, size_t dst_channel,
                    DeviceTask *src, size_t src_channel)
{ // ensure channel indexes are valid
  asynq  *s,*d;
  
  // alloc in/out channels if neccessary
  if( src->out == NULL )
    src->out = vector_PASYNQ_alloc(src_channel + 1);
  if( dst->in == NULL )
    dst->in = vector_PASYNQ_alloc(dst_channel + 1);    
  Guarded_Assert( src->out && dst->in );
  
  if( src_channel < src->out->nelem )                // source channel exists
  { s = src->out->contents[src_channel];
  
    if( dst_channel < dst->in->nelem )
    { asynq **d = dst->in->contents + dst_channel;
      Asynq_Unref( *d );
      *d = Asynq_Ref( s ); 
    } else
    { vector_PASYNQ_request( dst->in, dst_channel );  // make space
      dst->in->contents[ dst_channel ] = Asynq_Ref( s );   
    }
  } else if( dst_channel < dst->in->nelem ) // dst exists, but not src
  { d = dst->in->contents[dst_channel];
    vector_PASYNQ_request( src->out, src_channel );   // make space
    src->out->contents[src_channel] = Asynq_Ref( d );    
  } else
  { error("In DeviceTask_Connect: Neither channel exists\r\n");
  }
}
