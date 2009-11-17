#include "stdafx.h"
#include "common.h"
#include "test-runner.h"

#define DEBUG_TIC_TOC_TIMER

//
// UTILITIES
//

TicTocTimer tic(void)
{ TicTocTimer t = {0,0};
  LARGE_INTEGER tmp;
  Guarded_Assert_WinErr( QueryPerformanceFrequency( &tmp ) );
  t.rate = tmp.QuadPart;
  Guarded_Assert_WinErr( QueryPerformanceCounter  ( &tmp ) );
  t.last = tmp.QuadPart;
  
#ifdef DEBUG_TIC_TOC_TIMER  
  //Guarded_Assert( t.rate > 0 );
  debug("Tic() timer frequency: %u Hz\r\n"
        "           resolution: %g ns\r\n"
        "               counts: %u\r\n",(u32) t.rate, 
                                        1e9/(double)t.rate, 
                                        (u32) t.last);
#endif  
  return t;
}

// - returns time since last in seconds
// - updates timer
double toc(TicTocTimer *t)
{ LARGE_INTEGER tmp;
  u64 now;
  double delta;
  Guarded_Assert_WinErr( QueryPerformanceCounter( &tmp ) );
  now = tmp.QuadPart;
  delta = ( now - t->last) / (double)t->rate;
  t->last = now;
  return delta;
}

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