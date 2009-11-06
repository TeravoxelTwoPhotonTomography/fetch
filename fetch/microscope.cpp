#include "stdafx.h"
#include "microscope.h"

#include "device-digitizer.h"

TYPE_VECTOR_DEFINE( pf_microscope_hold_callback );
TYPE_VECTOR_DEFINE( pf_microscope_off_callback );

vector_pf_microscope_hold_callback g_microscope_hold_callbacks;
vector_pf_microscope_off_callback g_microscope_off_callbacks;

static unsigned int _free_g_microscope_hold_callbacks(void)
{ debug("Microscope application shutdown - free hold callbacks\r\n");
  vector_pf_microscope_hold_callback_free_contents( &g_microscope_hold_callbacks ); 
  return 0;
}

static unsigned int _free_g_microscope_off_callbacks(void)
{ debug("Microscope application shutdown - free off callbacks\r\n");
  vector_pf_microscope_off_callback_free_contents( &g_microscope_off_callbacks ); 
  return 0;
}

void Microscope_Register_Application_Shutdown_Procs(void)
{ Register_New_Shutdown_Callback( _free_g_microscope_hold_callbacks );
  Register_New_Shutdown_Callback( _free_g_microscope_off_callbacks  );
}

int Register_New_Microscope_Hold_Callback( pf_microscope_hold_callback callback )
{ int idx = g_microscope_hold_callbacks.count++;
  vector_pf_microscope_hold_callback_request( &g_microscope_hold_callbacks, idx );
  g_microscope_hold_callbacks.contents[idx] = callback;
  return idx;
}

int Register_New_Microscope_Off_Callback( pf_microscope_off_callback callback )
{ int idx = g_microscope_off_callbacks.count++;
  vector_pf_microscope_off_callback_request( &g_microscope_off_callbacks, idx );
  g_microscope_off_callbacks.contents[idx] = callback;
  return idx;
}

unsigned int Microscope_Off( void )
{ size_t cnt = g_microscope_off_callbacks.count;
  unsigned int err = 0;
  pf_microscope_off_callback *beg = g_microscope_off_callbacks.contents,
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

unsigned int Microscope_Hold( void )
{ size_t cnt = g_microscope_hold_callbacks.count;
  unsigned int err = 0;
  pf_microscope_hold_callback *beg = g_microscope_hold_callbacks.contents,
                              *cur = beg+cnt;

  static int lock = 0; // Avoid recursion
  assert( lock == 0 ); 
  lock = 1;

  while( cur-- > beg )
    if(cur)
      err |= (*cur)();
  lock = 0;

  debug("Microscope State: Hold\r\n");
  return err;
}

void Microscope_Application_Start(void)
{ Microscope_Register_Application_Shutdown_Procs();

  debug("Microscope application start\r\n");
  // TODO: Register devices here
  //       Devices should register shutdown procedures,
  //                               state procedures (off and hold),
  //                               and initialize themselves.
  Digitizer_Init();

  Microscope_Off();
  Microscope_Hold();
}