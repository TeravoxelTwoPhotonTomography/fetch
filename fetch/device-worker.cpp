#include "stdafx.h"
#include "microscope.h"
#include "device-worker.h"
#include "device-task.h"
#include "device-task-worker.h"

#define WORKER_DEFAULT_TIMEOUT                    INFINITE
#define WORKER_DEFAULT_INDEX_CAPACITY             4

TYPE_VECTOR_DEFINE(Worker_Index_Item);

vector_Worker_Index_Item *gv_workers = NULL;

LPCRITICAL_SECTION    gp_worker_index_lock  = NULL;

LPCRITICAL_SECTION
_worker_get_index_critical_section(void)
{ static CRITICAL_SECTION cs;
  if( !gp_worker_index_lock )
  { InitializeCriticalSectionAndSpinCount( &cs, 0x80000400 );
    gp_worker_index_lock = &cs;
  }
  return gp_worker_index_lock;
}

unsigned int Workers_Destroy_All(void)
{ LPCRITICAL_SECTION cs = _worker_get_index_critical_section();
  EnterCriticalSection(cs);
  
  Worker_Index_Item *beg = gv_workers->contents,
                    *cur = beg + gv_workers->count;
  while( cur-- > beg )
  { if( !Device_Disarm( cur->device, WORKER_DEFAULT_TIMEOUT ) )
      warning("Could not cleanly release Worker Device for alias %s.\r\n",cur->alias);
    Device_Free( cur->device );      
  }
  
  LeaveCriticalSection(cs);   
  return 0;
}

void 
_worker_module_safe_init(void)
{ if( !gv_workers ) // index not initialized yet so neither is anything else.
  { Guarded_Assert( 
      gv_workers = vector_Worker_Index_Item_alloc( WORKER_DEFAULT_INDEX_CAPACITY ) );

    // Register Shutdown functions - these get called in reverse order
    Register_New_Shutdown_Callback( &Workers_Destroy_All );

  }
}

Worker_Index_Item*
_worker_index_lookup( const char* alias )
{ LPCRITICAL_SECTION cs = _worker_get_index_critical_section();
  Worker_Index_Item *beg, *cur;
  
  EnterCriticalSection(cs);
  { beg = gv_workers->contents;
    cur = beg + gv_workers->count;
    while( cur-- > beg )
      if( strcmp( alias, cur->alias )==0 )
        break;
    if( cur < beg )
      cur = NULL;
  }
  LeaveCriticalSection(cs);
  return cur;
}

Worker*
Worker_Lookup(const char* alias)
{ Worker_Index_Item *item = _worker_index_lookup( alias );
  return (item) ? &item->worker:NULL;
}

Device*
Worker_Get_Device(Worker* self)
{ Worker_Index_Item *item = gv_workers->contents + self->index;
  Guarded_Assert( self->index <  gv_workers->count );
  Guarded_Assert( self->alias == item->alias );
  return item->device;
}

Worker* Worker_Init(const char* alias)
{ LPCRITICAL_SECTION cs = _worker_get_index_critical_section();
  Worker_Index_Item *item;
  
  _worker_module_safe_init();
  
  Guarded_Assert( _worker_index_lookup(alias) == NULL ); // ensure alias hasn't been added
  
  EnterCriticalSection(cs);
  { vector_Worker_Index_Item_request( gv_workers, ++gv_workers->count );                      // Alloc (append to end)
    item = gv_workers->contents + gv_workers->count - 1;
    
    item->worker.index = gv_workers->count - 1;                                                // Construct
    item->worker.alias = item->alias;
    
    memcpy(item->alias, alias, sizeof(char)*MIN( strlen(alias), WORKER_ALIAS_LENGTH ));
    
    item->device = Device_Alloc();
    item->device->context = NULL;
  }
  LeaveCriticalSection(cs);
  
  Device_Set_Available( item->device );
  return &item->worker;
}

unsigned int
_worker_device_disarm_unlocked(Worker_Index_Item *item)
{ unsigned int sts = 0; //error
  DeviceTask *task = NULL;
  
  debug("Worker: alias - %s\r\n"
        "\tAttempting to disarm.\r\n", item->alias );

  // Flush
  { asynq **beg =       item->device->task->in->contents,
          **cur = beg + item->device->task->in->nelem;
    while(cur-- > beg)
      Asynq_Flush_Waiting_Consumers( cur[0] );
  }
  
  task = item->device->task;
  //Device_Unlock( item->device );
  if( !Device_Disarm( item->device, WORKER_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly disarm worker for alias %s.\r\n",item->alias);
  //Device_Lock( item->device );
  
  // kill the task
  DeviceTask_Free( task );

  sts = 1;  // success
  debug("Worker: Disarmed alias %s\r\n",item->alias);
//Error:  
  return sts;
}

unsigned int
Worker_Destroy(Worker *self)
{ LPCRITICAL_SECTION cs = _worker_get_index_critical_section();
  Worker_Index_Item *item;

  EnterCriticalSection(cs);
  { item = gv_workers->contents + self->index;
    goto_if_fail( item->device != NULL, DeviceIsNullError );
    
    //Device_Lock( item->device ); // Don't think I need these bc I'm in a
                                   // critical section.
    { goto_if_fail( 1==_worker_device_disarm_unlocked(item), DeviceDisarmError );
    }
    //Device_Unlock( item->device );
    Device_Free( item->device );
    item->device = NULL;
  }
  LeaveCriticalSection(cs);
  return 0; // success
DeviceIsNullError:
  LeaveCriticalSection(cs);
  warning("Attempted to destroy a Worker who's device instance didn't exist.\r\n");
  return 1; // failure
DeviceDisarmError:
  //Device_Unlock( item->device );
  LeaveCriticalSection(cs);
  warning("Failed to disarm a Worker device.\r\n");
  return 1; // failure
}


//
// Workers
//

Device*
Worker_Compose_Averager_f32( const char *alias, Device *source, int ichan, int ntimes )
{ Worker   *worker = Worker_Init(alias);
  Device     *dest = Worker_Get_Device(worker);
  DeviceTask *task = Worker_Create_Task_Averager_f32(dest,ntimes);
  DeviceTask_Connect( task, 0, source->task, ichan);
  Device_Arm( dest, task, INFINITE );
  Device_Run( dest );
  return dest;
}

Device*
Worker_Compose_Caster( const char *alias, Device *source, int ichan, Basic_Type_ID source_type, Basic_Type_ID dest_type)
{ Worker   *worker = Worker_Init(alias);
  Device     *dest = Worker_Get_Device(worker);
  DeviceTask *task = Worker_Create_Task_Caster(dest,source_type,dest_type);
  DeviceTask_Connect( task, 0, source->task, ichan );
  Guarded_Assert( Device_Arm( dest, task,INFINITE ));
  Guarded_Assert( Device_Run( dest ));
  return dest;
}
