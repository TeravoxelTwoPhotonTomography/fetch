#include "stdafx.h"
#include "util-niscope.h"
#include "microscope.h"
#include "device-digitizer.h"

#define CheckWarn( expression )  (niscope_chk( g_digitizer.vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( g_digitizer.vi, expression, #expression, &error   ))

Digitizer            g_digitizer        = DIGITIZER_EMPTY;
Digitizer_Config     g_digitizer_config = DIGITIZER_CONFIG_DEFAULT;

unsigned int Digitizer_Destroy(void)
{ if(g_digitizer.lock)
  { DeleteCriticalSection( g_digitizer.lock );
    g_digitizer.lock = NULL;
  }
  return 0;
}

inline void
Digitizer_Lock(void)
{ EnterCriticalSection( g_digitizer.lock );
}

inline void
Digitizer_Unlock(void)
{ LeaveCriticalSection( g_digitizer.lock );
}

void Digitizer_Init(void)
{ //Synchronization
  static CRITICAL_SECTION cs;
  if( !g_digitizer.lock )
    InitializeCriticalSection(g_digitizer.lock = &cs);    
  g_digitizer.notify_available = CreateEvent( NULL,    // default security attributes
                                              TRUE,    // requires manual reset
                                              FALSE,   // initially unsignalled
                                              NULL );  // no name

  // Register Shutdown functions - these get called in reverse order
  Register_New_Shutdown_Callback( &Digitizer_Destroy );
  Register_New_Shutdown_Callback( &Digitizer_Close   );
  
  // Register Microscope state functions
  Register_New_Microscope_Hold_Callback( &Digitizer_Hold );
  Register_New_Microscope_Off_Callback( &Digitizer_Off );
}



unsigned int Digitizer_Close(void)
{ Digitizer_Lock();
  debug("Digitizer: Close.\r\n");
  { ViSession vi = g_digitizer.vi;
    ViStatus status = VI_SUCCESS;
    
    // Device is unavailable
    Guarded_Assert_WinErr(                          // This doesn't stop others from trying to use the
      ResetEvent( g_digitizer.notify_available ));  // device.  It's just so there is an option to wait
                                                    // till it's available.
    // Close the session
    if (vi)
      status = CheckWarn( niScope_close (vi) );
    g_digitizer.vi = NULL;
    
    Digitizer_Unlock();
    return status;
  }
}

unsigned int Digitizer_Off(void)
{ debug("Digitizer: Off\r\n");
  return Digitizer_Close();
}

unsigned int Digitizer_Hold(void)
{ Digitizer_Lock();
  ViStatus status = VI_SUCCESS;
  debug("Digitizer: Hold\r\n");
  { ViSession *vi = &g_digitizer.vi;

    if( (*vi) == NULL )
    { // Open the NI-SCOPE instrument handle
      status = CheckPanic (
        niScope_init (g_digitizer_config.resource_name, 
                      NISCOPE_VAL_TRUE,  // ID Query
                      NISCOPE_VAL_FALSE, // Reset?
                      vi)                // Session
      );
    }    
  }
  debug("\tGot session %3d with status %d\n",
  g_digitizer.vi,status);
  Digitizer_Unlock();
  return status;
}

//
// TESTING
//

void Digitizer_Append_Menu( HMENU menu )
{ HMENU submenu = CreatePopupMenu();
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_OFF,  "&Off"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_HOLD, "&Hold"));
  Guarded_Assert_WinErr( AppendMenu(    menu, MF_STRING | MF_POPUP, (UINT) submenu, "&Digitizer"));
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
