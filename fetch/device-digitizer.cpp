#include "stdafx.h"
#include "util-niscope.h"
#include "microscope.h"
#include "device-digitizer.h"
#include "device.h"

#define CheckWarn( expression )  (niscope_chk( g_digitizer.vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( g_digitizer.vi, expression, #expression, &error   ))

Digitizer             g_digitizer              = DIGITIZER_EMPTY;
Device               *gp_digitizer_device      = NULL;

unsigned int Digitizer_Destroy(void)
{ if( !Device_Release( gp_digitizer_device, DIGITIZER_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly release digitizer.\r\n");
  Device_Free( gp_digitizer_device );
  return 0;
}

void Digitizer_Init(void)
{ Guarded_Assert( gp_digitizer_device = Device_Alloc() );
  gp_digitizer_device->context = (void*) &g_digitizer;

  // Register Shutdown functions - these get called in reverse order
  Register_New_Shutdown_Callback( &Digitizer_Destroy );
  Register_New_Shutdown_Callback( &Digitizer_Close   );
  
  // Register Microscope state functions
  Register_New_Microscope_Hold_Callback( &Digitizer_Hold );
  Register_New_Microscope_Off_Callback( &Digitizer_Off );
}

unsigned int Digitizer_Close(void)
{ Device_Lock( gp_digitizer_device );
  debug("Digitizer: Close.\r\n");
  if( !Device_Release( gp_digitizer_device, DIGITIZER_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly release digitizer.\r\n");
  { ViSession vi = g_digitizer.vi;
    ViStatus status = VI_SUCCESS;
    // Close the session
    if (vi)
      status = CheckWarn( niScope_close (vi) );
    g_digitizer.vi = NULL;
    return status;
  }
  Device_Unlock( gp_digitizer_device );
}

unsigned int Digitizer_Off(void)
{ debug("Digitizer: Off\r\n");
  return Digitizer_Close();
}

unsigned int Digitizer_Hold(void)
{ Device_Lock( gp_digitizer_device );
  ViStatus status = VI_SUCCESS;
  debug("Digitizer: Hold\r\n");
  { ViSession *vi = &g_digitizer.vi;

    if( (*vi) == NULL )
    { // Open the NI-SCOPE instrument handle
      status = CheckPanic (
        niScope_init (g_digitizer.config.resource_name, 
                      NISCOPE_VAL_TRUE,  // ID Query
                      NISCOPE_VAL_FALSE, // Reset?
                      vi)                // Session
      );
    }
    niscope_debug_list_devices();
  }
  debug("\tGot session %3d with status %d\n",g_digitizer.vi,status);
  Device_Unlock( gp_digitizer_device );
  return status;
}

//
// TESTING
//

void Digitizer_Append_Menu( HMENU menu )
{ HMENU submenu = CreatePopupMenu();
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_OFF,  "&Off"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_HOLD, "&Hold"));
  Guarded_Assert_WinErr( AppendMenu(    menu, MF_STRING | MF_POPUP, (UINT_PTR) submenu, "&Digitizer"));
}

// This has the type of a WinProc
// Returns:
//    0  if the message was handled
//    1  otherwise
LRESULT CALLBACK Digitizer_Menu_Handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
    case IDM_DIGITIZER_OFF:
      { debug( "IDM_DIGITIZER_OFF\r\n" );
        Digitizer_Off();
      }
			break;
    case IDM_DIGITIZER_HOLD:
      { debug("IDM_DIGITIZER_HOLD\r\n");        
        Digitizer_Hold();
      }
      break;
		default:
			return 1;
		}
		break;
	default:
		return 1;
	}
	return 0;
}
