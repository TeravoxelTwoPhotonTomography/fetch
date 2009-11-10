#include "stdafx.h"
#include "util-niscope.h"
#include "microscope.h"
#include "device-digitizer.h"

#define CheckWarn( expression )  (niscope_chk( g_digitizer.vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( g_digitizer.vi, expression, #expression, &error   ))

Digitizer            g_digitizer        = {0};
//Digitizer_Config     g_digitizer_config = DIGITIZER_CONFIG_DEFAULT;
Digitizer_Config     g_digitizer_config =   {"Dev6\0",
                       DIGITIZER_MAX_SAMPLE_RATE,
                       1024,
                       0.0,
                       {{"0\0",2.0,NISCOPE_VAL_DC,VI_TRUE},
                        {"1\0",2.0,NISCOPE_VAL_DC,VI_TRUE},
                        {"2\0",2.0,NISCOPE_VAL_DC,VI_TRUE},
                        {"3\0",2.0,NISCOPE_VAL_DC,VI_TRUE},
                        {"4\0",2.0,NISCOPE_VAL_DC,VI_FALSE},
                        {"5\0",2.0,NISCOPE_VAL_DC,VI_FALSE},
                        {"6\0",2.0,NISCOPE_VAL_DC,VI_FALSE},
                        {"7\0",2.0,NISCOPE_VAL_DC,VI_FALSE}}
                       };

void Digitizer_Init(void)
{ // Register Shutdown function
  Register_New_Shutdown_Callback( &Digitizer_Close );
  Register_New_Microscope_Hold_Callback( &Digitizer_Hold );
  Register_New_Microscope_Off_Callback( &Digitizer_Off );
}

unsigned int Digitizer_Close(void)
{ debug("Digitizer: Close.\r\n");
  ViSession vi = g_digitizer.vi;
  ViStatus status = VI_SUCCESS;
  // Close the session
  if (vi)
    status = CheckWarn( niScope_close (vi) );  

  g_digitizer.vi = NULL;
  return status;
}

unsigned int Digitizer_Off(void)
{ debug("Digitizer: Off\r\n");
  return Digitizer_Close();
}

unsigned int Digitizer_Hold(void)
{ ViSession *vi = &g_digitizer.vi;
  ViStatus status = VI_SUCCESS;

  debug("Digitizer: Hold\r\n");
  if( (*vi) == NULL )
  { // Open the NI-SCOPE instrument handle
    status = CheckPanic (
      niScope_init (g_digitizer_config.resource_name, 
                    NISCOPE_VAL_TRUE,  // ID Query
                    NISCOPE_VAL_FALSE, // Reset?
                    vi)                // Session
    );
  }
  debug("\tGot session %3d with status %d\n",*vi,status);
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
