#include "stdafx.h"
#include "device-task.h"

TYPE_VECTOR_DEFINE( PASYNQ );

DWORD WINAPI Device_Task_Thread_Proc( LPVOID lpParam )
{ Device        *d = (Device*) lpParam;
  DeviceTask *task = d->task;
  return (task->run_proc)( d, task->in, task->out );
}

void
_devicetask_free_inputs( DeviceTask* self )
{ if( self->in )
  { size_t n = self->in->count;
    while(n--)
      Asynq_Unref( self->in->contents[n] );
    vector_PASYNQ_free( self->in );
    self->in = NULL;
  }
}

void
_devicetask_free_outputs( DeviceTask* self )
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
DeviceTask_Configure_Inputs( DeviceTask* self,
                             size_t  num_inputs,
                             size_t *input_queue_size, 
                             size_t *input_buffer_size)
{ if( num_inputs )
  { _devicetask_free_inputs( self );
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
DeviceTask_Configure_Outputs( DeviceTask* self,
                              size_t  num_outputs,
                              size_t *output_queue_size, 
                              size_t *output_buffer_size)
{ if( num_outputs )
  { _devicetask_free_outputs( self );
    self->out  = vector_PASYNQ_alloc( num_outputs ); 
    while( num_outputs-- )
      self->out->contents[num_outputs] 
          = Asynq_Alloc( output_queue_size[num_outputs],
                         output_buffer_size[num_outputs] );
    self->out->count = self->out->nelem; // resizable, so use count rather than nelem alone.
  }                                      //            start full
}

DeviceTask*
DeviceTask_Alloc( fp_device_task_cfg_proc cfg, 
                  fp_device_task_run_proc run )
{ DeviceTask* self = (DeviceTask*)Guarded_Calloc(1, sizeof(DeviceTask),"DeviceTask_Alloc");
  
  self->in = self->out = NULL;
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
    _devicetask_free_inputs( self );
    _devicetask_free_outputs( self );
  }
}