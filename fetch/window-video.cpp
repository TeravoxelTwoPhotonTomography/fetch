#include "stdafx.h"
#include "window-video.h"

static HWND         g_hVideo = NULL;

#define MAX_LOADSTRING 100

// Video_WndProc
LRESULT CALLBACK Video_WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{ int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

    case WM_CLOSE:
      { ShowWindow( hWnd, FALSE );
      }
	    break;
	    
    case WM_DESTROY:
      { PostQuitMessage(0);
      }
		  break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

ATOM Video_RegisterClass ( HINSTANCE hInstance )
{   //
    // Register class
    //
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = Video_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_FETCH);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = VIDEO_CLASS_NAME;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_FETCH);
    if( !RegisterClassEx(&wcex) )
    return FALSE;
}

HWND Video_InitInstance ( HINSTANCE hInstance, int nCmdShow)
{ HWND hWnd;   
  TCHAR        szTitle[MAX_LOADSTRING] = "Video:\0";  // The title bar text
	 
  LoadString(hInstance, IDS_APP_TITLE, szTitle+6, MAX_LOADSTRING-6);   
  //
  // Create window
  //  
  RECT rc = { 0, 0, 640, 480 };
  AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
  g_hVideo = CreateWindow( VIDEO_CLASS_NAME, 
                           "Direct3D 10 Tutorial 0: Setting Up Window",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT, 
                           rc.right - rc.left,
                           rc.bottom - rc.top, 
                           NULL,
                           NULL,
                           hInstance,
                           NULL);

  if( !g_hVideo )
      return FALSE;

  ShowWindow( g_hVideo, nCmdShow );
}

DWORD WINAPI Video_Main_Thread( LPVOID lparam )
{	MSG msg = {0};

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}