// fetch.cpp : Defines the entry point for the application.
//
// [ ] See digitizer control.  How should app get default device for a certain
//     category?

#include "stdafx.h"
#include "fetch.h"
#include "logger.h"
#include "devices/microscope.h"
#include "tasks/microscope-interaction.h"

#include "window-video.h"
#include "ui/PockelsSpinnerControl.h"
#include "ui/DigitizerStateMenu.h"
#include "ui/MicroscopeStateMenu.h"
#include "ui/ScannerStateMenu.h"

#define MAX_LOADSTRING 100

using namespace fetch;

// Global Variables:
HINSTANCE hInst;                         // current instance
TCHAR     szTitle[MAX_LOADSTRING];       // The title bar text
TCHAR     szWindowClass[MAX_LOADSTRING]; // the main window class name

HWND g_hwndLogger = NULL; // Logger window
HWND g_hwndVideo = NULL;  // Video  window

device::Microscope*             gp_microscope;
task::microscope::Interaction   g_microscope_default_task;

// Forward declarations of functions included in this code module:
ATOM             MyRegisterClass  (HINSTANCE hInstance);
BOOL             InitInstance     (HINSTANCE, int);
LRESULT CALLBACK WndProc          (HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About            (HWND, UINT, WPARAM, LPARAM);

unsigned int KillMicroscope(void)
{ if(gp_microscope) delete gp_microscope;
  return 0;
}

void ApplicationStart(HINSTANCE hInstance)
{ Register_New_Shutdown_Callback(Video_Display_Release);
  Register_New_Shutdown_Callback(KillMicroscope);
  
  // Setup logging
  Logger_Register_Default_Reporting();
  Reporting_Setup_Log_To_VSDebugger_Console();
  Reporting_Setup_Log_To_Filename("lastrun.log");

  Logger_RegisterClass(hInstance);
  g_hwndLogger = Logger_InitInstance(hInstance, SW_HIDE );
  g_hwndVideo  = Video_Display_Attach(hInstance, SW_HIDE );
  

  gp_microscope = new device::Microscope();  
  Guarded_Assert( gp_microscope->attach());  
  Guarded_Assert( gp_microscope->arm(&g_microscope_default_task,INFINITE));  // ok to do non-blocking here bc there's no way a task thread is running.
  
  Video_Display_Connect_Device( &gp_microscope->cast_to_i16, 0 );
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  MSG msg = { 0 };
  HACCEL hAccelTable;


  
  ApplicationStart(hInstance);
  
  // Initialize global strings
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadString(hInstance, IDC_FETCH, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);

  fetch::ui::pockels::PockelsIntensitySpinControl::Spinner_RegisterClass(hInstance);

  // Perform application initialization:
  if (!InitInstance(hInstance, nCmdShow))
  {
    return FALSE;
  }
 
  hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FETCH));

  // Main message loop    
  while (WM_QUIT != msg.message)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE ))
    {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    } else
    {
      Video_Display_Render_One_Frame();
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

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FETCH));
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wcex.lpszMenuName = MAKEINTRESOURCE(IDC_FETCH);
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
      NULL, // parent
      NULL, // menu
      hInstance, // instance
      NULL); // lparam
  if (!hWnd)
  {
    ReportLastWindowsError();
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
  //  static fetch::ui::pockels::UIControl ctl_pock = { 0, 0, 0, 0 };
  static ui::pockels::PockelsIntensitySpinControl *ctl_pockels = NULL;
  static ui::MicroscopeStateMenu *ctl_microscope = NULL;
  static ui::ScannerStateMenu    *ctl_scanner    = NULL;  
  //static fetch::ui::digitizer::Menu digitizer_menu("&Digitizer", Digitizer()/*replace*/);
  if(ctl_microscope==NULL)
  { ctl_microscope = new ui::MicroscopeStateMenu(gp_microscope);
    ctl_scanner    = new ui::ScannerStateMenu(&gp_microscope->scanner);
    ctl_pockels    = new ui::pockels::PockelsIntensitySpinControl((device::Pockels*)&gp_microscope->scanner);
  }

  if (!command_menu)
    command_menu = GetSubMenu(GetMenu(hWnd), 1);

  if (!ctl_microscope->Handler(hWnd, message, wParam, lParam))  
    return 0; // message was handled so return
  if (!ctl_scanner->Handler(hWnd, message, wParam, lParam))  
    return 0; // message was handled so return

  //if( !digitizer_menu.Handler(hWnd, message, wParam, lParam) )
  //   return 0; // message was handled so return

  // Top level handler    
  switch (message)
  {
    case WM_CREATE:
    {
      HMENU menu = GetMenu(hWnd);
      Guarded_Assert_WinErr( menu );
      ctl_microscope->Insert( menu, GetMenuItemCount(menu)-1, MF_BYPOSITION );
      ctl_scanner->Insert( menu, GetMenuItemCount(menu)-1, MF_BYPOSITION );
      //Scanner_UI_Insert_Menu(menu, GetMenuItemCount(menu) - 1, MF_BYPOSITION );
      //digitizer_menu->Insert( menu, GetMenuItemCount(menu)-1, MF_BYPOSITION );

      //
      // make the pockels control
      //
      ctl_pockels->Spinner_CreateControl(hWnd, 0, 0, IDC_POCKELS );
    }
      break;
    case WM_INITMENU:
    case WM_INITMENUPOPUP:
    {
      HMENU hmenu = (HMENU) wParam;
      if (hmenu == command_menu)
        Guarded_Assert_WinErr(
            -1!=CheckMenuItem(command_menu,
                IDM_CONTROL_LOGGER,
                MF_BYCOMMAND
                | (IsWindowVisible(g_hwndLogger)?MF_CHECKED:MF_UNCHECKED) ));
    }
      break;
    case WM_COMMAND:
      wmId = LOWORD(wParam);
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
        {
          if (!IsWindowVisible(g_hwndLogger) || IsIconic(g_hwndLogger))
            ShowWindow(g_hwndLogger, SW_SHOWNORMAL );
          else
            ShowWindow(g_hwndLogger, SW_HIDE );
          UpdateWindow(g_hwndLogger);
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
      return (INT_PTR) TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
      {
        EndDialog(hDlg, LOWORD(wParam));
        return (INT_PTR) TRUE;
      }
      break;
  }
  return (INT_PTR) FALSE;
}
