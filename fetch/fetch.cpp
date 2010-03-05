// fetch.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "fetch.h"
#include "logger.h"
#include "microscope.h"
#include "device-digitizer.h"
#include "device-scanner.h"

#include "window-video.h"
#include "window-control-pockels.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;				        		  		// current instance
TCHAR szTitle[MAX_LOADSTRING];		  			// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

HWND g_hwndLogger = NULL;                 // Logger window
HWND g_hwndVideo  = NULL;                 // Video  window

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void ApplicationStart(HINSTANCE hInstance)
{
  Logger_RegisterClass(hInstance);
  g_hwndLogger = Logger_InitInstance( hInstance, SW_HIDE );
  g_hwndVideo  = Video_Display_Attach( hInstance, SW_HIDE );

  Microscope_Application_Start(); // Registers devices and
                                  // Puts microscope into default holding state  
  Register_New_Shutdown_Callback( Video_Display_Release );

}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg = {0};
	HACCEL hAccelTable;

  // Setup logging
  Logger_Register_Default_Reporting();
  Reporting_Setup_Log_To_VSDebugger_Console();
  Reporting_Setup_Log_To_Filename( "lastrun.log" );

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FETCH, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	
  ui::pockels::RegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{ return FALSE;
	}

  ApplicationStart(hInstance);

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FETCH));

  // Main message loop    
  while( WM_QUIT != msg.message )
  { if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    { if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
      { TranslateMessage( &msg );
        DispatchMessage( &msg );
      }     
    } else
    { Video_Display_Render_One_Frame();
    }
  }	
  
  return Shutdown_Soft() | (unsigned int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			           = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	         = WndProc;
	wcex.cbClsExtra		         = 0;
	wcex.cbWndExtra		         = 0;
	wcex.hInstance		         = hInstance;
	wcex.hIcon		             = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FETCH));
	wcex.hCursor		           = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	       = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	         = MAKEINTRESOURCE(IDC_FETCH);
	wcex.lpszClassName	       = szWindowClass;
	wcex.hIconSm	            = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable


   hWnd = CreateWindow(szWindowClass, 
                       szTitle, 
                       WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT, 0, 
                       320, 240, 
                       NULL,         // parent
                       NULL,         // menu
                       hInstance,    // instance
                       NULL);        // lparam
   if (!hWnd)
   {  ReportLastWindowsError();
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	static HMENU command_menu = 0;
	static ui::pockels::UIControl ctl_pock = {0,0,0,0};
	
	if( !command_menu )
	  command_menu = GetSubMenu( GetMenu(hWnd), 1);

  if( !Scanner_UI_Handler(hWnd, message, wParam, lParam) )
     return 0; // message was handled so return

  //if( !Digitizer_UI_Handler(hWnd, message, wParam, lParam) )
  //   return 0; // message was handled so return

  // Top level handler    
	switch (message)
	{
  case WM_CREATE:
    { HMENU menu = GetMenu( hWnd );
      Guarded_Assert_WinErr( menu );
      Scanner_UI_Insert_Menu( menu, GetMenuItemCount(menu)-1, MF_BYPOSITION );
      //Digitizer_UI_Insert_Menu( menu, GetMenuItemCount(menu)-1, MF_BYPOSITION );
      
      //
      // make the pockels control
      //
      ctl_pock = ui::pockels::CreateControl( hWnd, 0, 0, IDC_POCKELS );
    }
    break;
	case WM_INITMENU:
	case WM_INITMENUPOPUP:
	  { HMENU hmenu = (HMENU) wParam;
	    if( hmenu == command_menu )
        Guarded_Assert_WinErr(
            -1!=CheckMenuItem(command_menu,
                          IDM_CONTROL_LOGGER,
                          MF_BYCOMMAND
                          | (IsWindowVisible(g_hwndLogger)?MF_CHECKED:MF_UNCHECKED) ));
	  }
	  break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_COMMAND_VIDEODISPLAY:
      Video_Display_GUI_On_ID_COMMAND_VIDEODISPLAY();
      break;
    case IDM_CONTROL_LOGGER:
      { if( !IsWindowVisible(g_hwndLogger) || IsIconic(g_hwndLogger) )
          ShowWindow(   g_hwndLogger, SW_SHOWNORMAL );
        else
          ShowWindow(   g_hwndLogger, SW_HIDE );
        UpdateWindow( g_hwndLogger );
      }
      break;
    case IDM_EXIT:
      DestroyWindow(g_hwndLogger);
      break;      
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
