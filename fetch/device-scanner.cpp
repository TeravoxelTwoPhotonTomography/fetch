#include "stdafx.h"
#include "util-niscope.h"
#include "microscope.h"
#include "device.h"
#include "device-scanner.h"
#include "device-task-scanner-video.h"

// Operational options
#if 0
#define SCANNER_NO_REGISTER_WITH_MICROSCOPE
#endif

// debug defines

#if 1
#define scanner_debug(...) debug(__VA_ARGS__)
#else
#define scanner_debug(...)
#endif

#define DIGWRN( expression )  (niscope_chk( g_scanner.digitizer->vi, expression, #expression, warning ))
#define DIGERR( expression )  (niscope_chk( g_scanner.digitizer->vi, expression, #expression, error   ))
#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)


typedef ViInt16 TPixel;

Scanner               g_scanner                = SCANNER_DEFAULT;
Device               *gp_scanner_device        = NULL;

DeviceTask           *gp_scanner_tasks[2]      = {NULL};
u32                   g_scanner_tasks_count    = 2;

DECLARE_USER_MESSAGE( IDM_SCANNER,              "{BF56ABFD-286B-4FC0-A25F-BFD7C236A13D}");
DECLARE_USER_MESSAGE( IDM_SCANNER_DETACH,       "{BF56ABFD-286B-4FC0-A25F-BFD7C236A13D}");
DECLARE_USER_MESSAGE( IDM_SCANNER_ATTACH,       "{BF56ABFD-286B-4FC0-A25F-BFD7C236A13D}");
DECLARE_USER_MESSAGE( IDM_SCANNER_TASK_STOP,    "{BF56ABFD-286B-4FC0-A25F-BFD7C236A13D}");
DECLARE_USER_MESSAGE( IDM_SCANNER_TASK_RUN,     "{BF56ABFD-286B-4FC0-A25F-BFD7C236A13D}");
DECLARE_USER_MESSAGE( IDM_SCANNER_TASK_0,       "{BF56ABFD-286B-4FC0-A25F-BFD7C236A13D}");
DECLARE_USER_MESSAGE( IDM_SCANNER_TASK_1,       "{BF56ABFD-286B-4FC0-A25F-BFD7C236A13D}");

unsigned int
_scanner_free_tasks(void)
{ u32 i = g_scanner_tasks_count;
  while(i--)
  { DeviceTask *cur = gp_scanner_tasks[i];
    DeviceTask_Free( cur );
    gp_scanner_tasks[i] = NULL;
  }
  return 0; //success
}

unsigned int Scanner_Destroy(void)
{ if( !Device_Disarm( gp_scanner_device, SCANNER_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly release scanner.\r\n");
  Device_Free( gp_scanner_device );
  return 0;
}

void Scanner_Init(void)
{ Guarded_Assert( gp_scanner_device = Device_Alloc() );
  gp_scanner_device->context = (void*) &g_scanner;

  // Register Shutdown functions - these get called in order
  Register_New_Shutdown_Callback( &Scanner_Detach );
  Register_New_Shutdown_Callback( &Scanner_Destroy );
  Register_New_Shutdown_Callback( &_scanner_free_tasks );
  
  Digitizer_Init();  // this registers it's own shutdown functions so must follow the scanner shutdown functions.
  
#ifndef DIGITIZER_NO_REGISTER_WITH_MICROSCOPE  
  // Register Microscope state functions
  Register_New_Microscope_Attach_Callback( &Scanner_Attach );
  Register_New_Microscope_Detach_Callback( &Scanner_Detach );
#endif
  // Create tasks
  gp_scanner_tasks[0] = Scanner_Create_Task_Video();
  gp_scanner_tasks[1] = Scanner_Create_Task_Line_Scan();
}

unsigned int Scanner_Detach(void)
{ unsigned int status = 1; // 0 indicates success, otherwise error
  
  if( g_scanner.digitizer )
    niScope_Abort( g_scanner.digitizer->vi ); // Abort any waiting fetches
  
  if( !Device_Disarm( gp_scanner_device, SCANNER_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly disarm scanner.\r\n");

  Device_Lock( gp_scanner_device );

  if( g_scanner.daq_clk )
  { debug("Scanner: Attempting to detach DAQ CLK channel. handle: 0x%p\r\n", g_scanner.daq_clk);
    DAQJMP( DAQmxStopTask(  g_scanner.daq_clk ));
    DAQJMP( DAQmxClearTask( g_scanner.daq_clk ));
  }
  if( g_scanner.daq_ao )
  { debug("Scanner: Attempting to detach DAQ AO  channel. handle: 0x%p\r\n", g_scanner.daq_ao );
    DAQJMP( DAQmxStopTask(  g_scanner.daq_ao ));
    DAQJMP( DAQmxClearTask( g_scanner.daq_ao ));
  }
  if( g_scanner.daq_shutter )
  { debug("Scanner: Attempting to detach DAQ AO  channel. handle: 0x%p\r\n", g_scanner.daq_shutter );
    DAQJMP( DAQmxStopTask(  g_scanner.daq_shutter ));
    DAQJMP( DAQmxClearTask( g_scanner.daq_shutter ));
  }

  status = 0;
  if( g_scanner.digitizer )
  { status = Digitizer_Detach();
    debug("Scanner: Digitizer detached with status %s.\r\n", status?"FAILURE":"SUCCESS");
  }

Error:  
  g_scanner.digitizer = NULL;
  g_scanner.daq_ao    = NULL;
  g_scanner.daq_clk   = NULL;
  gp_scanner_device->is_available = 0;  
  Device_Unlock( gp_scanner_device );
  return status;
}

DWORD WINAPI
_Scanner_Detach_thread_proc( LPVOID lparam )
{ return Scanner_Detach();
}

unsigned int
Scanner_Detach_Nonblocking(void)
{ return QueueUserWorkItem( _Scanner_Detach_thread_proc, NULL, NULL );
}

unsigned int Scanner_Attach(void)
{ unsigned int status = 0; // success 0, error 1
  Device_Lock( gp_scanner_device );
  debug("Scanner: Attach\r\n");
  goto_if( status = Digitizer_Attach(), Error );
  g_scanner.digitizer = Digitizer_Get();

  Guarded_Assert( g_scanner.daq_ao  == NULL );
  Guarded_Assert( g_scanner.daq_clk == NULL );
  Guarded_Assert( g_scanner.daq_shutter == NULL );

  status = DAQERR( DAQmxCreateTask( "galvo-command", &g_scanner.daq_ao ));
  status = DAQERR( DAQmxCreateTask( "scanner-clock", &g_scanner.daq_clk));
  status = DAQERR( DAQmxCreateTask( "shutter-command", &g_scanner.daq_shutter));

  Device_Set_Available( gp_scanner_device );
Error:
  Device_Unlock( gp_scanner_device );
  return status;
}

//
// UTILITIES
//

extern inline Scanner*
Scanner_Get(void)
{ return &g_scanner;
}

extern inline Device*
Scanner_Get_Device(void)
{ return gp_scanner_device;
}

extern inline DeviceTask*
Scanner_Get_Default_Task(void)
{ return gp_scanner_tasks[0];
}

//
// USER INTERFACE (WINDOWS)
//

typedef struct _scanner_ui_main_menu_state
{ HMENU menu,
        taskmenu;
} scanner_ui_main_menu_state;

scanner_ui_main_menu_state g_scanner_ui_main_menu_state = {0};

HMENU 
_scanner_ui_make_menu(void)
{ HMENU submenu = CreatePopupMenu(),
        taskmenu = CreatePopupMenu();
  Guarded_Assert_WinErr( AppendMenu( taskmenu, MF_STRING, IDM_SCANNER_TASK_0, "&Video" ));
  Guarded_Assert_WinErr( AppendMenu( taskmenu, MF_STRING, IDM_SCANNER_TASK_1, "&Line Scan" ));
                                       
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_SCANNER_DETACH,  "&Detach"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_SCANNER_ATTACH, "&Attach"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING | MF_POPUP,  (UINT_PTR) taskmenu, "&Tasks"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_SCANNER_TASK_RUN,  "&Run" ));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_SCANNER_TASK_STOP, "&Stop" ));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_SEPARATOR, NULL, NULL ));
  
  g_scanner_ui_main_menu_state.menu     = submenu;
  g_scanner_ui_main_menu_state.taskmenu = taskmenu;
  
  return submenu;
}

void 
Scanner_UI_Append_Menu( HMENU menu )
{ HMENU submenu = _scanner_ui_make_menu();  
  Guarded_Assert_WinErr( AppendMenu( menu, MF_STRING | MF_POPUP, (UINT_PTR) submenu, "&Scanner")); 
}

// Inserts the scanner menu before the specified menu item
// uFlags should be either MF_BYCOMMAND or MF_BYPOSITION.
void 
Scanner_UI_Insert_Menu( HMENU menu, UINT uPosition, UINT uFlags )
{ HMENU submenu = _scanner_ui_make_menu();  
  Guarded_Assert_WinErr( 
    InsertMenu( menu,
                uPosition,
                uFlags | MF_STRING | MF_POPUP,
                (UINT_PTR) submenu,
                "&Scanner")); 
}

// This has the type of a WinProc
// Returns:
//    0  if the message was handled
//    1  otherwise
LRESULT CALLBACK
Scanner_UI_Handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_INITMENU:
	case WM_INITMENUPOPUP:
	  { HMENU hmenu = (HMENU) wParam;
	    if( hmenu == g_scanner_ui_main_menu_state.menu )
	    { Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 1 /*Attach*/, MF_BYPOSITION
	                        | ( (g_scanner.digitizer==0)?MF_ENABLED:MF_GRAYED) ));
	      Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 2 /*Tasks*/, MF_BYPOSITION
	                        | ( (g_scanner.digitizer==0)?MF_GRAYED:MF_ENABLED) ));
	      Guarded_Assert_WinErr(-1!=
	        CheckMenuItem( hmenu, 2 /*Tasks*/, MF_BYPOSITION
	                        | ( Device_Is_Armed(gp_scanner_device)?MF_CHECKED:MF_UNCHECKED) ));
        Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION
	                        | ( (Device_Is_Armed(gp_scanner_device))?MF_ENABLED:MF_GRAYED) ));
	      Guarded_Assert_WinErr(-1!=
	        CheckMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION
	                        | ( Device_Is_Running(gp_scanner_device)?MF_CHECKED:MF_UNCHECKED) ));
        Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 4 /*Stop*/, MF_BYPOSITION
	                        | ( (Device_Is_Running(gp_scanner_device))?MF_ENABLED:MF_GRAYED) ));	                        
//TODO: update for running and stop
	      Guarded_Assert(-1!=                                                   // Detect state
	        CheckMenuRadioItem(g_scanner_ui_main_menu_state.menu,             // Toggle radio button to reflect
                             0,1,((g_scanner.digitizer==0)?0/*Off*/:1/*Hold*/), MF_BYPOSITION ) );
	    } else if( hmenu == g_scanner_ui_main_menu_state.taskmenu )
	    { // - Identify if any tasks are running.
	      //   If there is one, check that one and gray them all out.
	      // - Assumes there are at least as many tasks allocated in gp_scanner_tasks
	      //   as there are menu items.
	      // - Assumes menu items are in indexed correspondence with gp_scanner_tasks
	      // - Assumes gp_scanner_tasks are allocated
	      int i,n;
	      i = n = GetMenuItemCount(hmenu);
	      debug("Task Menu: found %d items\r\n",n);
	      if( Device_Is_Armed( gp_scanner_device ) || Device_Is_Running( gp_scanner_device ) )
	      { while(i--) // Search for armed task
	          if( gp_scanner_device->task == gp_scanner_tasks[i] )
	            break;
	        Guarded_Assert(-1!=
	          CheckMenuRadioItem(hmenu,0,n-1,i, MF_BYPOSITION ) );
	      }	else {
	        //clear the radio button indicator
	        Guarded_Assert(-1!=
	          CheckMenuRadioItem(hmenu,0,n-1,-1, MF_BYPOSITION ) );
	      }      
	      { // Disable/Enable all tasks if Running/Not running
	        int i = GetMenuItemCount(hmenu);
	        while(i--)
	          Guarded_Assert_WinErr(-1!=
	            EnableMenuItem( hmenu, i, MF_BYPOSITION 
	                            | (Device_Is_Running(gp_scanner_device)?MF_GRAYED:MF_ENABLED)));
	      }
	    }
	    return 1; // return 1 so that init menu messages are still processed by parent
	  }
	  break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		if( wmId == IDM_SCANNER_DETACH )
	  { debug( "IDM_SCANNER_DETACH\r\n" );
      Scanner_Detach_Nonblocking();
      
    } else if( wmId == IDM_SCANNER_ATTACH )
    { debug("IDM_SCANNER_ATTACH\r\n");        
      Scanner_Attach();
      
    } else if( wmId == IDM_SCANNER_TASK_0 )
    { debug("IDM_SCANNER_TASK_0\r\n");
      Device_Arm_Nonblocking( gp_scanner_device, gp_scanner_tasks[0], SCANNER_DEFAULT_TIMEOUT );
      
    } else if( wmId == IDM_SCANNER_TASK_1 )
    { debug("IDM_SCANNER_TASK_1\r\n");
      Device_Arm_Nonblocking( gp_scanner_device, gp_scanner_tasks[1], SCANNER_DEFAULT_TIMEOUT );
      
    } else if( wmId == IDM_SCANNER_TASK_RUN )
    { debug("IDM_SCANNER_RUN\r\n");      
      Device_Run_Nonblocking(gp_scanner_device);
      
    } else if( wmId == IDM_SCANNER_TASK_STOP )
    { debug("IDM_SCANNER_STOP\r\n");
      Device_Stop_Nonblocking( gp_scanner_device, SCANNER_DEFAULT_TIMEOUT );
      
    } else
      return 1;
		break;
	default:
		return 1;
	}
	return 0;
}
