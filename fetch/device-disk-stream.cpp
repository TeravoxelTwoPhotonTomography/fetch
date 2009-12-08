#include "stdafx.h"
#include "microscope.h"
#include "device-disk-stream.h"

//
// NOTES
// -----
//
//
// TODO
// ----
// [ ] Would be better to have a linked list or a proper hash table rather than the
//     expanding vector for the index of source.
//

#define DISK_STREAM_DEFAULT_TIMEOUT                    INFINITE
#define DISK_STREAM_DEFAULT_INDEX_CAPACITY             4

TYPE_VECTOR_DEFINE(Disk_Stream_Index_Item);

vector_Disk_Stream_Index_Item *gv_streams = NULL;

LPCRITICAL_SECTION    gp_disk_stream_index_lock  = NULL;
DeviceTask           *gp_disk_stream_tasks[1]    = {NULL};
u32                   g_disk_stream_tasks_count  = 1;

LPCRITICAL_SECTION
_disk_stream_get_index_critical_section(void)
{ static CRITICAL_SECTION cs;
  if( !gp_disk_stream_index_lock )
  { InitializeCriticalSectionAndSpinCount( &cs, 0x80000400 );
    gp_disk_stream_index_lock = &cs;
  }
  return gp_disk_stream_index_lock;
}

//unsigned int
//_disk_stream_free_tasks(void)
//{ u32 i = g_disk_stream_tasks_count;
//  while(i--)
//  { DeviceTask *cur = gp_disk_stream_tasks[i];
//    DeviceTask_Free( cur );
//    gp_disk_stream_tasks[i] = NULL;
//  }
//  return 0; //success
//}

unsigned int Disk_Stream_Destroy(void)
{ LPCRITICAL_SECTION cs = _disk_stream_get_index_critical_section();
  EnterCriticalSection(cs);
  
  Disk_Stream_Index_Item *beg = gv_streams->contents,
                         *cur = beg + gv_streams->count;
  while( cur-- > beg )
  { if( !Device_Disarm( cur->device, DISK_STREAM_DEFAULT_TIMEOUT ) )
      warning("Could not cleanly release Disk Stream Device for alias %s.\r\n",cur->alias);
    Device_Free( cur->device );      
  }
  
  LeaveCriticalSection(cs);   
  return 0;
}

void Disk_Stream_Init(void)
{ Guarded_Assert(
    gv_streams = vector_Disk_Stream_Index_Item_alloc( DISK_STREAM_DEFAULT_INDEX_CAPACITY ) );

  // Register Shutdown functions - these get called in reverse order
  //Register_New_Shutdown_Callback( &_disk_stream_free_tasks );
  Register_New_Shutdown_Callback( &Disk_Stream_Destroy );
  Register_New_Shutdown_Callback( &Disk_Stream_Detach_All );
  
  // Register Microscope state functions
  //XXX Register_New_Microscope_Attach_Callback( &Disk_Stream_Attach );
  Register_New_Microscope_Detach_Callback   ( &Disk_Stream_Detach_All );
  
  // Create tasks
  //gp_disk_stream_tasks[0] = Disk_Stream_Create_Raw_Write_Task();
}

Disk_Stream_Index_Item*
_disk_stream_index_lookup( const char* alias )
{ LPCRITICAL_SECTION cs = _disk_stream_get_index_critical_section();
  Disk_Stream_Index_Item *beg, *cur;
  
  EnterCriticalSection(cs);
  { beg = gv_streams->contents;
    cur = beg + gv_streams->count;
    while( cur-- > beg )
      if( strcmp( alias, cur->alias )==0 )
        break;
    if( cur < beg )
      cur = NULL;
  }
  LeaveCriticalSection(cs);
  return cur;
}

// ------
// DETACH
// ------

// Returns 1 on success, and 0 otherwise
// A failure probably means the CloseHandle function failed.
//
// If callers are closing more than one file at a time, they
// should try to close as many files as possible before 
// panicing due to an error.
//
// This function will not panic.
unsigned int 
_disk_stream_detach_unlocked(Disk_Stream_Index_Item* item)
{ unsigned int sts = 0; //error
  
  debug("Disk Stream: alias - %s\r\n"
        "\tAttempting to close file: %s\r\n", item->alias, item->item.path );

  // Flush
  { asynq **beg =       item->device->task->in->contents,
          **cur = beg + item->device->task->in->nelem;
    while(cur-- > beg)
      Asynq_Flush_Waiting_Consumers( cur[0] );
  }
  
  Device_Unlock( item->device );
  if( !Device_Disarm( item->device, DISK_STREAM_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly disarm disk stream for alias %s.\r\n",item->alias);
  Device_Lock( item->device );
  
  // kill the task
  DeviceTask_Free( item->device->task );
  item->device->task = NULL;
  
  // Close the file  
  { HANDLE *ph = &item->item.hfile;
    if( *ph != INVALID_HANDLE_VALUE)
    { if(!CloseHandle( *ph ))
      { ReportLastWindowsError();
        goto Error;
      }
      *ph = NULL;
    }
  }

  sts = 1;  // success
  debug("Disk Stream: Detached alias %s\r\n",item->alias);
Error:  
  return sts;
}

unsigned int 
Disk_Stream_Detach(const char* alias)
{ unsigned int status = 1; //error
  Disk_Stream_Index_Item* item = NULL;
  Guarded_Assert( item = _disk_stream_index_lookup(alias) );
  
  Device_Lock( item->device );
  status = 1!=_disk_stream_detach_unlocked(item);
  Device_Unlock( item->device );
  return status;
}

DWORD WINAPI
_disk_stream_detach_thread_proc( LPVOID lparam )
{ asynq *q = (asynq*) lparam;
  Disk_Stream_Index_Item *item;
  unsigned int status = 1; //error
  if(!Asynq_Pop_Copy_Try(q,&item))
  { warning("In Disk_Stream_Detach_Nonblocking work procedure:\r\n"
            "\tCould not pop arguments from queue.\r\n"
            "\tThis means a request got lost somewhere.\r\n");
    return 1;
  }  
  Device_Lock( item->device );
  status = 1!=_disk_stream_detach_unlocked(item);
  Device_Unlock( item->device );
  return status;
}

unsigned int
Disk_Stream_Detach_Nonblocking(const char* alias)
{ static asynq *q = NULL;
  Disk_Stream_Index_Item *item = _disk_stream_index_lookup(alias);
  if(!item)
  { warning("In Disk_Stream_Detach_Nonblocking:\r\n"
            "\tCould not find item for alias %s\r\n",alias);
    return 0;
  }
  
  if(!q)
    q = Asynq_Alloc(32, sizeof(Disk_Stream_Index_Item*) );
  if( !Asynq_Push_Copy(q, &item, TRUE /*expand queue when full*/) )
  { warning("In Disk_Stream_Detach_Nonblocking: Could not push request arguments to queue.");
    return 0;
  }
  
  return QueueUserWorkItem( _disk_stream_detach_thread_proc, (void*)q, NULL );
}

unsigned int 
Disk_Stream_Detach_All(void)
{ LPCRITICAL_SECTION cs = _disk_stream_get_index_critical_section();
  unsigned int ok = 1;
  EnterCriticalSection(cs);
  { Disk_Stream_Index_Item *beg = gv_streams->contents,
                           *end = beg + gv_streams->count,
                           *cur = end;
    while( cur-- > beg ) 
    { Device_Lock( cur->device );
      ok &= _disk_stream_detach_unlocked(cur);
      Device_Unlock( cur->device );
    }
  }
  LeaveCriticalSection(cs);
  return !ok;
}

DWORD WINAPI
_disk_stream_detach_all_thread_proc( LPVOID lparam )
{ return Disk_Stream_Detach_All();
}

unsigned int
Disk_Stream_Detach_All_Nonblocking(void)
{ return QueueUserWorkItem( _disk_stream_detach_all_thread_proc, NULL, NULL );
}

// ------
// ATTACH
// ------

unsigned int 
Disk_Stream_Attach (const char* alias,     // look-up name for the stream
                    const char* filename,  // path to file
                    char mode)             // can be 'r' or 'w'
{ Disk_Stream_Index_Item *item;
  LPCRITICAL_SECTION cs = _disk_stream_get_index_critical_section();
  int sts = 1; //success

  Guarded_Assert( _disk_stream_index_lookup(alias) == NULL ); // ensure alias hasn't been added
    
  EnterCriticalSection(cs);
  
  // Alloc and get item
  vector_Disk_Stream_Index_Item_request( gv_streams, ++gv_streams->count );
  item = gv_streams->contents + gv_streams->count - 1;
  
  // Construct the device and disk stream
  memcpy(item->alias, alias, sizeof(char)*MIN( strlen(alias), 32 ));
  item->device = Device_Alloc();
  item->device->context = (void*)&item->item;
  memcpy( item->item.path, filename, sizeof(char)*strlen(filename) );
  item->item.mode = mode;  
  
  // Open the file
  // Associate read/write task
  { DWORD desired_access,
          share_mode,
          creation_disposition,
          flags_and_attr;        
    switch( mode )
    { case 'r':
        desired_access             =     GENERIC_READ;
        share_mode                 =     FILE_SHARE_READ;       //other processes can read
        creation_disposition       =     OPEN_EXISTING;
        flags_and_attr             =     0;
        Guarded_Assert(
          item->device->task = Disk_Stream_Create_Raw_Read_Task());
        break;
      case 'w':
        desired_access             =     GENERIC_WRITE;
        share_mode                 =     0;                     //don't share
        creation_disposition       =     CREATE_ALWAYS;
        flags_and_attr             =     FILE_ATTRIBUTE_NORMAL;
        Guarded_Assert(
          item->device->task = Disk_Stream_Create_Raw_Write_Task());
        break;
      default:
       Guarded_Assert( mode == 'r' || mode == 'w' ); // a self documenting assert error
    }
    item->item.hfile = CreateFile(filename, 
                                  desired_access,
                                  share_mode,
                                  NULL,
                                  creation_disposition,
                                  flags_and_attr,
                                  NULL );
  }
  if( item->item.hfile==0 )
  { ReportLastWindowsError();
    warning("Could not open file for alias %s\r\n"
            "\tat %s\r\n"
            "\twith mode %c\r\n",alias, filename, mode);
    Device_Free( item->device );
    gv_streams->count--;
    item->device = NULL;
    memset(item->alias,0,sizeof(item->alias));
    sts = 0; //failure
  } else 
  { Device_Set_Available(item->device);
  }
  LeaveCriticalSection(cs);
  return sts;
}

// ---------
// Utilities
// ---------

unsigned int
Disk_Stream_Connect_To_Input (const char* alias, Device *source_device, int channel)
{ Disk_Stream_Index_Item *item;  
  Guarded_Assert(item = _disk_stream_index_lookup(alias));
  DeviceTask_Connect( item->device->task, 0, source_device->task, channel );
  return 1;
}

Device*
Disk_Stream_Get_Device(const char* alias)
{ Disk_Stream_Index_Item *item;
  Guarded_Assert(  item = _disk_stream_index_lookup(alias)  );
  return item->device;
}

// -----                                                 
// Tasks
// -----

// Write
unsigned int
_Disk_Stream_Task_Write_Cfg( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ return 1;
}

unsigned int
_Disk_Stream_Task_Write_Proc( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ asynq *q  = in->contents[0];
  void *buf = Asynq_Token_Buffer_Alloc(q);
  DWORD nbytes = q->q->buffer_size_bytes,
        written;
  Disk_Stream *stream = (Disk_Stream*) d->context;
  TicTocTimer t = tic();
  do
  { if( Asynq_Pop(q, &buf) )
    { double dt = toc(&t);
      debug("Writing %s bytes: %d\r\n"
            "\t%-7.1f bytes per second (dt: %f)\r\n", 
            stream->path, nbytes, nbytes/dt, dt );
      WriteFile( stream->hfile, buf, nbytes, &written, NULL );
      Guarded_Assert( written == nbytes );
    }
  } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
  Asynq_Token_Buffer_Free(buf); 
  return 1;
}

DeviceTask*
Disk_Stream_Create_Raw_Write_Task(void)
{ return DeviceTask_Alloc(_Disk_Stream_Task_Write_Cfg,
                          _Disk_Stream_Task_Write_Proc);
}

// Read
unsigned int
_Disk_Stream_Task_Read_Cfg( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ return 1;
}

unsigned int
_Disk_Stream_Task_Read_Proc( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ asynq *q  = out->contents[0];
  void *buf = Asynq_Token_Buffer_Alloc(q);
  DWORD nbytes = q->q->buffer_size_bytes,
        bytes_read;
  Disk_Stream *stream = (Disk_Stream*) d->context;
  TicTocTimer t = tic();
  do  
  { double dt;
    Guarded_Assert_WinErr(
      ReadFile( stream->hfile, buf, nbytes, &bytes_read, NULL ));  
    Guarded_Assert( Asynq_Push_Timed(q, &buf, DISK_STREAM_DEFAULT_TIMEOUT) );
    dt = toc(&t);
    debug("Read %s bytes: %d\r\n"
          "\t%-7.1f bytes per second (dt: %f)\r\n", 
          stream->path, nbytes, nbytes/dt, dt );
  } while ( nbytes && WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
  Asynq_Token_Buffer_Free(buf); 
  return 1;
}

DeviceTask*
Disk_Stream_Create_Raw_Read_Task(void)
{ return DeviceTask_Alloc(_Disk_Stream_Task_Read_Cfg,
                          _Disk_Stream_Task_Read_Proc);
}