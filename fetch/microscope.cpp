#include "stdafx.h"
#include "microscope.h"

#include "device-digitizer.h"
#include "device-galvo-mirror.h"
#include "device-disk-stream.h"

TYPE_VECTOR_DEFINE( pf_microscope_attach_callback );
TYPE_VECTOR_DEFINE( pf_microscope_detach_callback );

vector_pf_microscope_attach_callback g_microscope_attach_callbacks;
vector_pf_microscope_detach_callback g_microscope_detach_callbacks;

static unsigned int _free_g_microscope_attach_callbacks(void)
{ debug("Microscope application shutdown - free attach callbacks\r\n");
  vector_pf_microscope_attach_callback_free_contents( &g_microscope_attach_callbacks ); 
  return 0;
}

static unsigned int _free_g_microscope_detach_callbacks(void)
{ debug("Microscope application shutdown - free off callbacks\r\n");
  vector_pf_microscope_detach_callback_free_contents( &g_microscope_detach_callbacks ); 
  return 0;
}

void Microscope_Register_Application_Shutdown_Procs(void)
{ Register_New_Shutdown_Callback( _free_g_microscope_attach_callbacks );
  Register_New_Shutdown_Callback( _free_g_microscope_detach_callbacks  );
}

size_t Register_New_Microscope_Attach_Callback( pf_microscope_attach_callback callback )
{ size_t idx = g_microscope_attach_callbacks.count++;
  vector_pf_microscope_attach_callback_request( &g_microscope_attach_callbacks, idx );
  g_microscope_attach_callbacks.contents[idx] = callback;
  return idx;
}

size_t Register_New_Microscope_Detach_Callback( pf_microscope_detach_callback callback )
{ size_t idx = g_microscope_detach_callbacks.count++;
  vector_pf_microscope_detach_callback_request( &g_microscope_detach_callbacks, idx );
  g_microscope_detach_callbacks.contents[idx] = callback;
  return idx;
}

unsigned int Microscope_Detach( void )
{ size_t cnt = g_microscope_detach_callbacks.count;
  unsigned int err = 0;
  pf_microscope_detach_callback *beg = g_microscope_detach_callbacks.contents,
                             *cur = beg+cnt;  

  static int lock = 0; // Avoid recursion
  assert( lock == 0 ); 
  
  lock = 1;
  while( cur-- > beg )
    if(cur)
      err |= (*cur)();
  lock = 0;

  debug("Microscope State: Off\r\n");
  return err;
}

unsigned int Microscope_Attach( void )
{ size_t cnt = g_microscope_attach_callbacks.count;
  unsigned int err = 0;
  pf_microscope_attach_callback *beg = g_microscope_attach_callbacks.contents,
                              *cur = beg+cnt;

  static int lock = 0; // Avoid recursion
  assert( lock == 0 ); 
  
  lock = 1;
  while( cur-- > beg )
    if(cur)
      err |= (*cur)();
  lock = 0;

  debug("Microscope State: Attached\r\n");
  return err;
}

void Microscope_Application_Start(void)
{ Microscope_Register_Application_Shutdown_Procs();

  debug("Microscope application start\r\n");
  // TODO: Register devices here
  //       Devices should register shutdown procedures,
  //                               state procedures (off and attach),
  //                               and initialize themselves.
  Digitizer_Init();
  Galvo_Mirror_Init();
  Disk_Stream_Init();
  
  Microscope_Detach();
  Microscope_Attach();
    
  Device_Arm( Digitizer_Get_Device(), 
              Digitizer_Get_Default_Task(),
              INFINITE );
  
  Disk_Stream_Attach("digitizer-frames","frames.raw",'w');
  { Device *stream = Disk_Stream_Get_Device( "digitizer-frames" );
    Guarded_Assert( Device_Arm( stream,
                                stream->task,
                                INFINITE ));
    Disk_Stream_Connect_To_Input("digitizer-frames",Digitizer_Get_Device(),0);
    Device_Run( stream );
  }
  
  Disk_Stream_Attach("digitizer-wfm",   "wfm.raw",'w');
  { Device *stream = Disk_Stream_Get_Device( "digitizer-wfm" );
    Guarded_Assert( Device_Arm( stream,
                                stream->task,
                                INFINITE ));
    Disk_Stream_Connect_To_Input("digitizer-wfm"   ,Digitizer_Get_Device(),1);
    Device_Run(stream);
  }
}