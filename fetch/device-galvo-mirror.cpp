#include "stdafx.h"
#include "device-galvo-mirror.h"
#include "device-task-galvo-mirror-cont-triangle-wave.h"
#include "microscope.h"

#define OnErrWarn(  expression )  (Guarded_DAQmx( expression, #expression, &warning ))
#define OnErrPanic( expression )  (Guarded_DAQmx( expression, #expression, &error   ))
#define OnErrGoto(  expression )   goto_if_fail( 0 == OnErrWarn(expression), Error )

#define GALVO_MIRROR_DEFAULT_TIMEOUT INFINITE

Galvo_Mirror          g_galvo_mirror              = GALVO_MIRROR_DEFAULT;
Device               *gp_galvo_mirror_device      = NULL;

DeviceTask           *gp_galvo_mirror_tasks[1]    = {NULL};
u32                   g_galvo_mirror_tasks_count  = 1;

DECLARE_USER_MESSAGE( IDM_GALVO_MIRROR              ,"{1AC60A65-813E-4fab-AA3D-A3E884104ACC}");
DECLARE_USER_MESSAGE( IDM_GALVO_MIRROR_DETACH       ,"{1AC60A65-813E-4fab-AA3D-A3E884104ACC}");
DECLARE_USER_MESSAGE( IDM_GALVO_MIRROR_ATTACH       ,"{1AC60A65-813E-4fab-AA3D-A3E884104ACC}");
DECLARE_USER_MESSAGE( IDM_GALVO_MIRROR_LIST_DEVICES ,"{1AC60A65-813E-4fab-AA3D-A3E884104ACC}");
DECLARE_USER_MESSAGE( IDM_GALVO_MIRROR_TASK_STOP    ,"{1AC60A65-813E-4fab-AA3D-A3E884104ACC}");
DECLARE_USER_MESSAGE( IDM_GALVO_MIRROR_TASK_RUN     ,"{1AC60A65-813E-4fab-AA3D-A3E884104ACC}");
DECLARE_USER_MESSAGE( IDM_GALVO_MIRROR_TASK_0       ,"{1AC60A65-813E-4fab-AA3D-A3E884104ACC}");

unsigned int
_galvo_mirror_free_tasks(void)
{ u32 i = g_galvo_mirror_tasks_count;
  while(i--)
  { DeviceTask *cur = gp_galvo_mirror_tasks[i];
    DeviceTask_Free( cur );
    gp_galvo_mirror_tasks[i] = NULL;
  }
  return 0; //success
}

unsigned int Galvo_Mirror_Destroy(void)
{ if( !Device_Disarm( gp_galvo_mirror_device, GALVO_MIRROR_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly release Galvo_Mirror.\r\n");
  Device_Free( gp_galvo_mirror_device );
  return 0;
}

void Galvo_Mirror_Init(void)
{ Guarded_Assert( gp_galvo_mirror_device = Device_Alloc() );
  gp_galvo_mirror_device->context = (void*) &g_galvo_mirror;

  // Register Shutdown functions - these get called in order
  Register_New_Shutdown_Callback( &Galvo_Mirror_Detach );
  Register_New_Shutdown_Callback( &Galvo_Mirror_Destroy );
  Register_New_Shutdown_Callback( &_galvo_mirror_free_tasks );
  
  // Register Microscope state functions
  Register_New_Microscope_Attach_Callback( &Galvo_Mirror_Attach );
  Register_New_Microscope_Detach_Callback( &Galvo_Mirror_Detach );
  
  // Create tasks
  gp_galvo_mirror_tasks[0] = Galvo_Mirror_Create_Task_Continuous_Scan_Immediate_Trigger();
}

unsigned int Galvo_Mirror_Detach(void)
{ unsigned int status = 1; //error
  
  debug("Galvo Mirror: Attempting to close task handle: 0x%p\r\n", g_galvo_mirror.task_handle);
  if( !Device_Disarm( gp_galvo_mirror_device, GALVO_MIRROR_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly disarm Galvo Mirror.\r\n");

  Device_Lock( gp_galvo_mirror_device );  
  if( g_galvo_mirror.task_handle)   // Close the session
    OnErrGoto( DAQmxClearTask( g_galvo_mirror.task_handle) );

  status = 0;  // success
  debug("Galvo_Mirror: Detached.\r\n");
Error:  
  g_galvo_mirror.task_handle = 0;
  Device_Unlock( gp_galvo_mirror_device );
  return status;
}

DWORD WINAPI
_galvo_mirror_detach_thread_proc( LPVOID lparam )
{ return Galvo_Mirror_Detach();
}

unsigned int
Galvo_Mirror_Detach_Nonblocking(void)
{ return QueueUserWorkItem( _galvo_mirror_detach_thread_proc, NULL, NULL );
}

unsigned int 
Galvo_Mirror_Attach(void)
{ Device_Lock( gp_galvo_mirror_device );
  unsigned int status = 0; /*success*/
  debug("Galvo_Mirror: Attach\r\n");
  { TaskHandle *pdc = &(g_galvo_mirror.task_handle);

    if( (*pdc) == NULL ) // Open the DAQmx task handle
    { status = OnErrPanic (
        DAQmxCreateTask(g_galvo_mirror.config.task_name,pdc)
      );
    }
  }
  debug("\tGot session 0x%p with status %d\n",g_galvo_mirror.task_handle,status);
  Device_Unlock( gp_galvo_mirror_device );

  return status;
}

//
// Utiltites
//

extern inline Galvo_Mirror*
Galvo_Mirror_Get(void)
{ return &g_galvo_mirror;
}

extern inline Device*
Galvo_Mirror_Get_Device(void)
{ return gp_galvo_mirror_device;
}

extern inline DeviceTask*
Galvo_Mirror_Get_Default_Task(void)
{ return gp_galvo_mirror_tasks[0];
}

//
// USER INTERFACE (WINDOWS)
//

typedef struct _galvo_mirror_ui_main_menu_state
{ HMENU menu,
        taskmenu;

} galvo_mirror_ui_main_menu_state;

galvo_mirror_ui_main_menu_state g_galvo_mirror_ui_main_menu_state = {0};

HMENU 
_galvo_mirror_ui_make_menu(void)
{ HMENU submenu = CreatePopupMenu(),
        taskmenu = CreatePopupMenu();
  Guarded_Assert_WinErr( AppendMenu( taskmenu, MF_STRING, IDM_GALVO_MIRROR_TASK_0, "Continuous &Scan" ));
  
                                       
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_GALVO_MIRROR_DETACH,  "&Detach"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_GALVO_MIRROR_ATTACH, "&Attach"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING | MF_POPUP,  (UINT_PTR) taskmenu, "&Tasks"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_GALVO_MIRROR_TASK_RUN,  "&Run" ));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_GALVO_MIRROR_TASK_STOP, "&Stop" ));  
  
  g_galvo_mirror_ui_main_menu_state.menu     = submenu;
  g_galvo_mirror_ui_main_menu_state.taskmenu = taskmenu;
  
  return submenu;
}

void 
Galvo_Mirror_UI_Append_Menu( HMENU menu )
{ HMENU submenu = _galvo_mirror_ui_make_menu();  
  Guarded_Assert_WinErr( AppendMenu( menu, MF_STRING | MF_POPUP, (UINT_PTR) submenu, "&Scan Mirror")); 
}

// Inserts the Galvo Mirror menu before the specified menu item
// uFlags should be either MF_BYCOMMAND or MF_BYPOSITION.
void 
Galvo_Mirror_UI_Insert_Menu( HMENU menu, UINT uPosition, UINT uFlags )
{ HMENU submenu = _galvo_mirror_ui_make_menu();  
  Guarded_Assert_WinErr( 
    InsertMenu( menu,
                uPosition,
                uFlags | MF_STRING | MF_POPUP,
                (UINT_PTR) submenu,
                "&Scan Mirror"));
}

// This has the type of a WinProc
// Returns:
//    0  if the message was handled
//    1  otherwise
LRESULT CALLBACK
Galvo_Mirror_UI_Handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_INITMENU:
	case WM_INITMENUPOPUP:
	  { HMENU hmenu = (HMENU) wParam;
	    if( hmenu == g_galvo_mirror_ui_main_menu_state.menu )
	    { Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 1 /*Attach*/, MF_BYPOSITION
	                        | ( (g_galvo_mirror.task_handle==0)?MF_ENABLED:MF_GRAYED) ));
	      Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 2 /*Tasks*/, MF_BYPOSITION
	                        | ( (g_galvo_mirror.task_handle==0)?MF_GRAYED:MF_ENABLED) ));
	      Guarded_Assert_WinErr(-1!=
	        CheckMenuItem( hmenu, 2 /*Tasks*/, MF_BYPOSITION
	                        | ( Device_Is_Armed(gp_galvo_mirror_device)?MF_CHECKED:MF_UNCHECKED) ));
        Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION
	                        | ( (Device_Is_Armed(gp_galvo_mirror_device))?MF_ENABLED:MF_GRAYED) ));
	      Guarded_Assert_WinErr(-1!=
	        CheckMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION
	                        | ( Device_Is_Running(gp_galvo_mirror_device)?MF_CHECKED:MF_UNCHECKED) ));
        Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 4 /*Stop*/, MF_BYPOSITION
	                        | ( (Device_Is_Running(gp_galvo_mirror_device))?MF_ENABLED:MF_GRAYED) ));	                        
//TODO: update for running and stop
	      Guarded_Assert(-1!=                                                      // Detect state
	        CheckMenuRadioItem(g_galvo_mirror_ui_main_menu_state.menu,             // Toggle radio button to reflect
                             0,1,((g_galvo_mirror.task_handle==0)?0/*Off*/:1/*Hold*/), MF_BYPOSITION ) );
	    } else if( hmenu == g_galvo_mirror_ui_main_menu_state.taskmenu )
	    { // - Identify if any tasks are running.
	      //   If there is one, check that one and gray them all out.
	      // - Assumes there are at least as many tasks allocated in gp_Galvo_Mirror_tasks
	      //   as there are menu items.
	      // - Assumes menu items are in indexed correspondence with gp_Galvo_Mirror_tasks
	      // - Assumes gp_Galvo_Mirror_tasks are allocated
	      int i,n;
	      i = n = GetMenuItemCount(hmenu);
	      debug("Task Menu: found %d items\r\n",n);
	      if( Device_Is_Armed( gp_galvo_mirror_device ) || Device_Is_Running( gp_galvo_mirror_device ) )
	      { while(i--) // Search for armed task
	          if( gp_galvo_mirror_device->task == gp_galvo_mirror_tasks[i] )
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
	                            | (Device_Is_Running(gp_galvo_mirror_device)?MF_GRAYED:MF_ENABLED)));
	      }
	    }
	    return 1; // return 1 so that init menu messages are still processed by parent
	  }
	  break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		if (wmId == IDM_GALVO_MIRROR_DETACH )
    { debug( "IDM_GALVO_MIRROR_DETACH\r\n" );
      Galvo_Mirror_Detach_Nonblocking();
      
    } else if (wmId == IDM_GALVO_MIRROR_ATTACH )
    { debug("IDM_GALVO_MIRROR_ATTACH\r\n");
      Galvo_Mirror_Attach();
      
    } else if (wmId == IDM_GALVO_MIRROR_TASK_0 )
    { debug("IDM_GALVO_MIRROR_TASK_0\r\n");
      Device_Arm_Nonblocking( gp_galvo_mirror_device, gp_galvo_mirror_tasks[0], GALVO_MIRROR_DEFAULT_TIMEOUT );        
      
    } else if (wmId == IDM_GALVO_MIRROR_TASK_RUN )
    { debug("IDM_GALVO_MIRROR_RUN\r\n");      
      Device_Run_Nonblocking(gp_galvo_mirror_device);
      
    } else if (wmId == IDM_GALVO_MIRROR_TASK_RUN )
    { debug("IDM_GALVO_MIRROR_RUN\r\n");      
      Device_Run_Nonblocking(gp_galvo_mirror_device);
      
    } else if (wmId == IDM_GALVO_MIRROR_TASK_STOP )
    { debug("IDM_GALVO_MIRROR_STOP\r\n");
      Device_Stop_Nonblocking( gp_galvo_mirror_device, GALVO_MIRROR_DEFAULT_TIMEOUT );\
      
    } else
      return 1;
		break;
	default:
		return 1;
	}
	return 0;
}
