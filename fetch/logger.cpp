// logger.cpp : Defines the entry point for the application.
//
// TODO: figure out how to propigate ctrl-s up through edit control

#include "stdafx.h"
#include "logger.h"

#include <assert.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

#undef  DEBUG_LOGGER_CONTROL
#undef  DEBUG_LOGGER_USE_BLOCKING_UPDATES

#define LOGGER_RICH_EDIT
#include <Richedit.h>  // #defines MSFTEDIT_CLASS and RICHEDIT_CLASS

#define MAX_LOADSTRING 100

TYPE_VECTOR_DECLARE( TCHAR );
TYPE_VECTOR_DEFINE( TCHAR );

// Global Variables:
static HWND         g_hLogger = NULL;
static vector_TCHAR g_Logger_Buffer = VECTOR_EMPTY;


//
//   FUNCTION: Logger_Update()
//
//   PURPOSE: Copies logger text buffer to the edit control for display.
//
//   COMMENTS:
//
//        Sends a WM_SETTEXT message to edit control.
//
void Logger_Update( void )
{ HWND    hedit  = GetDlgItem( g_hLogger, IDC_LOGGER_EDIT );
  
  if(hedit)
    Guarded_Assert_WinErr( SendMessage( hedit,                                 // Can't use PostMessage here
                                        WM_SETTEXT,                            // because the message requires access to 
                                        NULL,                                  // a pointer that's sometimes in another 
                                        (LPARAM) g_Logger_Buffer.contents ));  // thread.
}

DWORD WINAPI _logger_update_thread_proc( LPVOID lparam )
{ Logger_Update(); return 0;
}

void Logger_Update_Nonblocking( void )
{ Guarded_Assert_WinErr(
    QueueUserWorkItem( _logger_update_thread_proc, NULL, NULL ));
}

//
//   FUNCTION: Logger_InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND Logger_InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;   
   TCHAR        szTitle[MAX_LOADSTRING] = "Log: ";  // The title bar text
	 
   LoadString(hInstance, IDS_APP_TITLE, szTitle+5, MAX_LOADSTRING-5);

   hWnd = CreateWindowEx( WS_EX_COMPOSITED,
                          LOGGER_CLASS_NAME, 
                          szTitle, 
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, 0, 
                          320, 240, 
                          NULL, 
                          NULL, 
                          hInstance, 
                          NULL);
   if (!hWnd)
   { ReportLastWindowsError();
     error("Could not instatiate logger window.\r\n");
   }

   g_hLogger = hWnd;
   Logger_Update();
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return hWnd;
}

void Logger_Push_Text( const TCHAR* msg )
{ size_t  len    = _tcslen(msg); 

  vector_TCHAR_request( &g_Logger_Buffer, 
                         g_Logger_Buffer.count + len );
#pragma warning( push )
#pragma warning( disable:4996 )
  //Requires compiling with Multiple byte character set (not unicode)
  strncpy( g_Logger_Buffer.contents + g_Logger_Buffer.count,          
           msg,
           len+1 );                // copy the null termination
#pragma warning( pop )
  g_Logger_Buffer.count += len;    // but don't add the null termination to the count

#ifdef DEBUG_LOGGER_USE_BLOCKING_UPDATES
  Logger_Update();
#else
  Logger_Update_Nonblocking();
#endif
}

//
//  FUNCTION: Logger_WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_CREATE
//  WM_SIZE
//  WM_COMMAND	- process the application menu
//  WM_DESTROY	- post a quit message and return
//  WM_NCDESTROY (conditional on an #ifdef )
//
//
LRESULT CALLBACK Logger_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	//PAINTSTRUCT ps;
	HDC hdc;

  static HWND  hEdit      = NULL;
  static HFONT hfDefault  = NULL;

	switch (message)
	{
  case WM_CREATE:
    { RECT rcClient;

#ifdef LOGGER_DOUBLE_BUFFERED_PAINTING
      Guarded_Assert( BufferedPaintInit() == S_OK );
#endif

      GetClientRect( hWnd, &rcClient );

#ifdef LOGGER_RICH_EDIT
      Guarded_Assert( LoadLibrary( TEXT("Msftedit.dll") ));
      Guarded_Assert(
        hEdit = CreateWindowEx( 0, //extended styles
                                TEXT("RICHEDIT50W"), //class name
                                TEXT("No messages have been posted to the logger."),
                                WS_CHILD 
                                | WS_VISIBLE
                                | WS_BORDER
                                | WS_TABSTOP
                                | WS_VSCROLL
                                | WS_HSCROLL 
                                | ES_MULTILINE 
                                | ES_AUTOHSCROLL 
                                | ES_AUTOVSCROLL
                                ,
                                0, //x                            
                                0, //y
                                rcClient.right,  //width
                                rcClient.bottom, //height
                                hWnd,
                                (HMENU) IDC_LOGGER_EDIT, // child window identifier
                                (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                                NULL )
      );
#else
      Guarded_Assert(
        hEdit = CreateWindowEx( WS_EX_CLIENTEDGE,
                                _T("EDIT"),
                                _T("No messages have been posted to the logger."),
                                WS_CHILD 
                                | WS_VISIBLE
                                | WS_VSCROLL
                                | WS_HSCROLL 
                                | ES_MULTILINE 
                                | ES_AUTOHSCROLL 
                                | ES_AUTOVSCROLL
                                ,
                                0, //x                            
                                0, //y
                                rcClient.right,  //width
                                rcClient.bottom, //height
                                hWnd,
                                (HMENU) IDC_LOGGER_EDIT, // child window identifier
                                (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
                                NULL )
      );
#endif

      // Set the font
      { long h;
        hdc = GetDC(NULL);
        h = -MulDiv( 8, GetDeviceCaps(hdc, LOGPIXELSY), 72 );
        ReleaseDC( NULL, hdc );
        if( hfDefault )
          DeleteObject( hfDefault );
        hfDefault = CreateFont( h, 0, // height and width
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                "Lucida Console" );
        if( !SendMessage( hEdit, WM_SETFONT, (WPARAM) hfDefault, TRUE ) )
          warning("Could not set font in logger edit control.\r\n");
          
      }

      // Set read only edit box
      Guarded_Assert( 
        SendMessage( hEdit, EM_SETREADONLY, TRUE, 0L )
        );

    }
    break;
  case WM_SIZE:
    { RECT rect;
      GetClientRect(hWnd, &rect);
      if( hEdit )      
        SetWindowPos( hEdit, NULL, 
                      0, 0, rect.right, rect.bottom, 
                      SWP_NOZORDER);      
    }
    break;
	case WM_COMMAND:
    { wmId    = LOWORD(wParam);
		  wmEvent = HIWORD(wParam);
      
		  // Parse the menu selections:
		  switch (wmId)
	    {
	    case IDM_LOG_CLOSE:
        { debug("IDM_LOG_CLOSE\r\n");
          ShowWindow( hWnd, FALSE );
        }
		    break;
		  case IDM_EXIT:
			  DestroyWindow(hWnd);
			  break;
      case IDM_LOG_CLEAR:
        { debug("IDM_LOG_CLEAR\r\n");
          ZeroMemory( g_Logger_Buffer.contents, g_Logger_Buffer.count );
          g_Logger_Buffer.count = 0;
          Logger_Update();
        }
        break;
      case IDM_CONTROL_LOGGER:
        { if( !IsWindowVisible(hWnd) || IsIconic(hWnd) )
            ShowWindow(   hWnd, SW_SHOWNORMAL );
          else
            ShowWindow(   hWnd, SW_HIDE );
          UpdateWindow( hWnd );
        }
        break;
      case IDM_LOG_SAVE:
        { OPENFILENAME ofn;
          char filename[MAX_PATH] = "\0";
#ifdef DEBUG_LOGGER_CONTROL
          debug("Got save command in logger\r\n");
#endif
          
          ZeroMemory( &ofn, sizeof( ofn ) );
          ofn.lStructSize = sizeof(ofn);
          ofn.hwndOwner   = hWnd;
          ofn.lpstrFile   = filename;
          ofn.lpstrFilter = "Log Files (*.log)\0*.log\0"
                            "Text Files (*.txt)\0*.txt\0"
                            "All Files (*.*)\0*.*\0";
          ofn.nMaxFile = MAX_PATH;
          ofn.Flags    = OFN_EXPLORER 
                         | OFN_HIDEREADONLY
                         | OFN_PATHMUSTEXIST
                         | OFN_NOREADONLYRETURN
                         | OFN_OVERWRITEPROMPT;
          ofn.lpstrDefExt = "log";

          if( GetSaveFileName(&ofn) )
          { 
#pragma warning(push)
#pragma warning( disable:4996 )
            FILE *fp = fopen(ofn.lpstrFile, "w");
#pragma warning(pop)
            fprintf( fp, g_Logger_Buffer.contents );
            fclose(fp);
#ifdef DEBUG_LOGGER_CONTROL
            debug("\tWrote to file: %s\r\n", ofn.lpstrFile );
#endif
          }
#ifdef DEBUG_LOGGER_CONTROL
          else 
          { debug("\tDid not write to file. Error: %d\r\n", CommDlgExtendedError());
          }
#endif          

        }
        break;
#ifdef DEBUG_LOGGER_CONTROL
      case IDC_LOGGER_EDIT:
        switch (wmEvent)
        { case EN_UPDATE: // 1024
            break;          
          default:
            debug("command: %5d %5d %d\r\n",wmId,wmEvent,((HWND)lParam)==hEdit);
        }
        break;
#endif
	    default:
#ifdef DEBUG_LOGGER_CONTROL
        debug("Default Processing: command %5d %5d\r\n",wmId,wmEvent);
#endif
		    return DefWindowProc(hWnd, message, wParam, lParam);
	    }
    }
		break;
#ifdef DEBUG_LOGGER_CONTROL
  case WM_NOTIFY:
    { debug("notify");
    }
    break;
#endif
  case WM_CLOSE:
    { ShowWindow( hWnd, SW_HIDE );
    }
    break;
	case WM_DESTROY:
    { PostQuitMessage(0);
    }
		break;

#ifdef LOGGER_DOUBLE_BUFFERED_PAINTING
  case WM_NCDESTROY:
    BufferedPaintUnInit();
    break;
#endif

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

//
//  FUNCTION: Logger_RegisterClass()
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
ATOM Logger_RegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style			  = 0; 
	wcex.lpfnWndProc	= Logger_WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			  = NULL;
	wcex.hCursor	  	= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_LOGGER);
  wcex.lpszClassName	= LOGGER_CLASS_NAME;
	wcex.hIconSm		  = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

void Logger_Register_Default_Reporting(void)
{ 
  Register_New_Error_Reporting_Callback( &Logger_Push_Text );
  Register_New_Debug_Reporting_Callback( &Logger_Push_Text );
  Register_New_Warning_Reporting_Callback( &Logger_Push_Text );
}
