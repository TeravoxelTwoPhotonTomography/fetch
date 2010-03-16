#include "stdafx.h"
#include "util-niscope.h"
#include "microscope.h"
#include "device-digitizer.h"
#include "device.h"
#include "device-task-digitizer-fetch-forever.h"

// Operational options
#if 0
#define DIGITIZER_NO_REGISTER_WITH_MICROSCOPE
#endif

// debug defines


#if 1
#define digitizer_debug(...) debug(__VA_ARGS__)
#else
#define digitizer_debug(...)
#endif

#define CheckWarn( expression )  (niscope_chk( g_digitizer.vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( g_digitizer.vi, expression, #expression, &error   ))
#define ViErrChk( expression )    goto_if( CheckWarn(expression), Error )

Digitizer             g_digitizer              = DIGITIZER_DEFAULT;
Device               *gp_digitizer_device      = NULL;

DeviceTask           *gp_digitizer_tasks[1]    = {NULL};
u32                   g_digitizer_tasks_count  = 1;

DECLARE_USER_MESSAGE( IDM_DIGITIZER,              "{5975C4F4-CA28-4d2b-A77C-B4D1C535BB21}");
DECLARE_USER_MESSAGE( IDM_DIGITIZER_DETACH,       "{5975C4F4-CA28-4d2b-A77C-B4D1C535BB21}");
DECLARE_USER_MESSAGE( IDM_DIGITIZER_ATTACH,       "{5975C4F4-CA28-4d2b-A77C-B4D1C535BB21}");
DECLARE_USER_MESSAGE( IDM_DIGITIZER_LIST_DEVICES, "{5975C4F4-CA28-4d2b-A77C-B4D1C535BB21}");
DECLARE_USER_MESSAGE( IDM_DIGITIZER_TASK_STOP,    "{5975C4F4-CA28-4d2b-A77C-B4D1C535BB21}");
DECLARE_USER_MESSAGE( IDM_DIGITIZER_TASK_RUN,     "{5975C4F4-CA28-4d2b-A77C-B4D1C535BB21}");
DECLARE_USER_MESSAGE( IDM_DIGITIZER_TASK_0,       "{5975C4F4-CA28-4d2b-A77C-B4D1C535BB21}");
DECLARE_USER_MESSAGE( IDM_DIGITIZER_TASK_1,       "{5975C4F4-CA28-4d2b-A77C-B4D1C535BB21}");

unsigned int
_digitizer_free_tasks(void)
{ u32 i = g_digitizer_tasks_count;
  while(i--)
  { DeviceTask *cur = gp_digitizer_tasks[i];
    DeviceTask_Free( cur );
    gp_digitizer_tasks[i] = NULL;
  }
  return 0; //success
}

unsigned int Digitizer_Destroy(void)
{ if( !Device_Disarm( gp_digitizer_device, DIGITIZER_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly release digitizer.\r\n");
  Device_Free( gp_digitizer_device );
  gp_digitizer_device = NULL;
  return 0;
}

void Digitizer_Init(void)
{ Guarded_Assert( gp_digitizer_device = Device_Alloc() );
  gp_digitizer_device->context = (void*) &g_digitizer;

  // Register Shutdown functions - these get called in order
  Register_New_Shutdown_Callback( &Digitizer_Detach );
  Register_New_Shutdown_Callback( &Digitizer_Destroy );
  Register_New_Shutdown_Callback( &_digitizer_free_tasks );

#ifndef DIGITIZER_NO_REGISTER_WITH_MICROSCOPE  
  // Register Microscope state functions
  Register_New_Microscope_Attach_Callback( &Digitizer_Attach );
  Register_New_Microscope_Detach_Callback( &Digitizer_Detach );
#endif
  
  // Create tasks
  gp_digitizer_tasks[0] = Digitizer_Create_Task_Fetch_Forever();
}

unsigned int Digitizer_Detach(void)
{ ViStatus status = 1; //error
  
  debug("Digitizer: Attempting to detach. vi: %d\r\n", g_digitizer.vi);
  if( !Device_Disarm( gp_digitizer_device, DIGITIZER_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly disarm digitizer.\r\n");

  Device_Lock( gp_digitizer_device );
  if( g_digitizer.vi)
    ViErrChk( niScope_close(g_digitizer.vi) );  // Close the session

  status = 0;  // success
  debug("Digitizer: Detached.\r\n");
Error:  
  g_digitizer.vi = 0;
  gp_digitizer_device->is_available = 0;  
  Device_Unlock( gp_digitizer_device );
  return status;
}

DWORD WINAPI
_Digitizer_Detach_thread_proc( LPVOID lparam )
{ return Digitizer_Detach();
}

unsigned int
Digitizer_Detach_Nonblocking(void)
{ return  QueueUserWorkItem( _Digitizer_Detach_thread_proc, NULL, NULL );
}

unsigned int Digitizer_Attach(void)
{ Device_Lock( gp_digitizer_device );
  ViStatus status = VI_SUCCESS;
  debug("Digitizer: Attach\r\n");
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
  }
  Device_Set_Available( gp_digitizer_device );
  debug("\tGot session %3d with status %d\n",g_digitizer.vi,status);
  Device_Unlock( gp_digitizer_device );

  return status;
}

//
// UTILITIES
//

extern inline Digitizer*
Digitizer_Get(void)
{ return &g_digitizer;
}

extern inline Device*
Digitizer_Get_Device(void)
{ return gp_digitizer_device;
}

extern inline DeviceTask*
Digitizer_Get_Default_Task(void)
{ return gp_digitizer_tasks[0];
}
//
// USER INTERFACE (WINDOWS)
//

typedef struct _digitizer_ui_main_menu_state
{ HMENU menu,
        taskmenu;

} digitizer_ui_main_menu_state;

digitizer_ui_main_menu_state g_digitizer_ui_main_menu_state = {0};

HMENU 
_digitizer_ui_make_menu(void)
{ HMENU submenu = CreatePopupMenu(),
        taskmenu = CreatePopupMenu();
  Guarded_Assert_WinErr( AppendMenu( taskmenu, MF_STRING, IDM_DIGITIZER_TASK_0, "Fetch &Forever" ));
  Guarded_Assert_WinErr( AppendMenu( taskmenu, MF_STRING, IDM_DIGITIZER_TASK_1, "Fetch &Triggered" ));
  
                                       
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_DETACH,  "&Detach"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_ATTACH, "&Attach"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING | MF_POPUP,  (UINT_PTR) taskmenu, "&Tasks"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_TASK_RUN,  "&Run" ));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_TASK_STOP, "&Stop" ));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_SEPARATOR, NULL, NULL ));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_LIST_DEVICES, "&List NI modular devices"));
  
  g_digitizer_ui_main_menu_state.menu     = submenu;
  g_digitizer_ui_main_menu_state.taskmenu = taskmenu;
  
  return submenu;
}

void 
Digitizer_UI_Append_Menu( HMENU menu )
{ HMENU submenu = _digitizer_ui_make_menu();  
  Guarded_Assert_WinErr( AppendMenu( menu, MF_STRING | MF_POPUP, (UINT_PTR) submenu, "&Digitizer")); 
}

// Inserts the digitizer menu before the specified menu item
// uFlags should be either MF_BYCOMMAND or MF_BYPOSITION.
void 
Digitizer_UI_Insert_Menu( HMENU menu, UINT uPosition, UINT uFlags )
{ HMENU submenu = _digitizer_ui_make_menu();  
  Guarded_Assert_WinErr( 
    InsertMenu( menu,
                uPosition,
                uFlags | MF_STRING | MF_POPUP,
                (UINT_PTR) submenu,
                "&Digitizer")); 
}

// This has the type of a WinProc
// Returns:
//    0  if the message was handled
//    1  otherwise
LRESULT CALLBACK
Digitizer_UI_Handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_INITMENU:
	case WM_INITMENUPOPUP:
	  { HMENU hmenu = (HMENU) wParam;
	    if( hmenu == g_digitizer_ui_main_menu_state.menu )
	    { Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 1 /*Attach*/, MF_BYPOSITION
	                        | ( (g_digitizer.vi==0)?MF_ENABLED:MF_GRAYED) ));
	      Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 2 /*Tasks*/, MF_BYPOSITION
	                        | ( (g_digitizer.vi==0)?MF_GRAYED:MF_ENABLED) ));
	      Guarded_Assert_WinErr(-1!=
	        CheckMenuItem( hmenu, 2 /*Tasks*/, MF_BYPOSITION
	                        | ( Device_Is_Ready(gp_digitizer_device)?MF_CHECKED:MF_UNCHECKED) ));
        Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION
	                        | ( (Device_Is_Ready(gp_digitizer_device))?MF_ENABLED:MF_GRAYED) ));
	      Guarded_Assert_WinErr(-1!=
	        CheckMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION
	                        | ( Device_Is_Running(gp_digitizer_device)?MF_CHECKED:MF_UNCHECKED) ));
        Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 4 /*Stop*/, MF_BYPOSITION
	                        | ( (Device_Is_Running(gp_digitizer_device))?MF_ENABLED:MF_GRAYED) ));	                        
//TODO: update for running and stop
	      Guarded_Assert(-1!=                                                   // Detect state
	        CheckMenuRadioItem(g_digitizer_ui_main_menu_state.menu,             // Toggle radio button to reflect
                             0,1,((g_digitizer.vi==0)?0/*Off*/:1/*Hold*/), MF_BYPOSITION ) );
	    } else if( hmenu == g_digitizer_ui_main_menu_state.taskmenu )
	    { // - Identify if any tasks are running.
	      //   If there is one, check that one and gray them all out.
	      // - Assumes there are at least as many tasks allocated in gp_digitizer_tasks
	      //   as there are menu items.
	      // - Assumes menu items are in indexed correspondence with gp_digitizer_tasks
	      // - Assumes gp_digitizer_tasks are allocated
	      int i,n;
	      i = n = GetMenuItemCount(hmenu);
	      debug("Task Menu: found %d items\r\n",n);
	      if( Device_Is_Ready( gp_digitizer_device ) || Device_Is_Running( gp_digitizer_device ) )
	      { while(i--) // Search for armed task
	          if( gp_digitizer_device->task == gp_digitizer_tasks[i] )
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
	                            | (Device_Is_Running(gp_digitizer_device)?MF_GRAYED:MF_ENABLED)));
	      }
	    }
	    return 1; // return 1 so that init menu messages are still processed by parent
	  }
	  break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		if( wmId == IDM_DIGITIZER_DETACH )
	  { debug( "IDM_DIGITIZER_DETACH\r\n" );
      Digitizer_Detach_Nonblocking();
      
    } else if( wmId == IDM_DIGITIZER_ATTACH )
    { debug("IDM_DIGITIZER_ATTACH\r\n");        
      Digitizer_Attach();
      
    } else if( wmId == IDM_DIGITIZER_LIST_DEVICES )
    { debug("IDM_DIGITIZER_LIST_DEVICES\r\n");
      niscope_debug_list_devices();
      
    } else if( wmId == IDM_DIGITIZER_TASK_0 )
    { debug("IDM_DIGITIZER_TASK_0\r\n");
      Device_Arm_Nonblocking( gp_digitizer_device, gp_digitizer_tasks[0], DIGITIZER_DEFAULT_TIMEOUT );
      
    } else if( wmId == IDM_DIGITIZER_TASK_RUN )
    { debug("IDM_DIGITIZER_RUN\r\n");      
      Device_Run_Nonblocking(gp_digitizer_device);
      
    } else if( wmId == IDM_DIGITIZER_TASK_STOP )
    { debug("IDM_DIGITIZER_STOP\r\n");
      Device_Stop_Nonblocking( gp_digitizer_device, DIGITIZER_DEFAULT_TIMEOUT );
      
    } else
      return 1;
		break;
	default:
		return 1;
	}
	return 0;
}
