#include "stdafx.h"
#include "device-task.h"

TYPE_VECTOR_DEFINE( PASYNQ );

DWORD WINAPI Device_Task_Thread_Proc( LPVOID lpParam )
{ Device        *d = (Device*) lpParam;
  DeviceTask *task = d->task;
  return (task->run_proc)( d, task->in, task->out );
}

DeviceTask*
DeviceTask_Alloc( size_t num_inputs,
                  size_t input_queue_size, 
                  size_t input_buffer_size,
                  size_t num_outputs,
                  size_t output_queue_size,
                  size_t output_buffer_size,
                  fp_device_task_cfg_proc cfg, 
                  fp_device_task_run_proc run )
{ DeviceTask* self = (DeviceTask*)Guarded_Calloc(1, sizeof(DeviceTask),"DeviceTask_Alloc");

  if( num_inputs )
  { self->in  = vector_PASYNQ_alloc( num_inputs  ); 
    while( num_inputs-- )
      self->in->contents[num_inputs] = Asynq_Alloc( input_queue_size, input_buffer_size );
    self->in->count = self->in->nelem; // resizable, so use count rather than nelem alone.
  }                                    //            start full
  
  if( num_outputs )
  { self->out  = vector_PASYNQ_alloc( num_outputs  ); 
    while( num_outputs-- )
      self->out->contents[num_outputs] = Asynq_Alloc( output_queue_size, output_buffer_size );
    self->out->count = self->out->nelem; // resizable, so use count rather than nelem alone.
  }                                      //            start full
  
  self->cfg_proc = cfg;
  self->run_proc = run;
  self->main     = &Device_Task_Thread_Proc;
  
  return self;
}

void
DeviceTask_Free( DeviceTask *self )
{ if(self)
  { self->cfg_proc = NULL;
    self->run_proc = NULL;
    if( self->in )
    { size_t n = self->in->count;
      while(n--)
        Asynq_Unref( self->in->contents[n] );
      vector_PASYNQ_free( self->in );
      self->in = NULL;
    }
    if( self->out )
    { size_t n = self->out->count;
      while(n--)
        Asynq_Unref( self->out->contents[n] );
      vector_PASYNQ_free( self->out );
      self->out = NULL;
    }
  }
}