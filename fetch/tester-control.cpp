#include "stdafx.h"
#include "tester-control.h"

//
// MENU CONTROLLER
// 
#if 0               
void Tester_Append_Menu( HMENU menu )
{ HMENU submenu = CreatePopupMenu();
  
  Guarded_Assert_WinErr( AppendMenu(    menu, MF_STRING | MF_POPUP, (UINT) submenu, "&Tester"));
}

// This has the type of a WinProc
// Returns:
//    0  if the message was handled
//    1  otherwise
LRESULT CALLBACK Tester_Menu_Handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
#endif