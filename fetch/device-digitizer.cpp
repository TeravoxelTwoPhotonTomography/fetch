#include "stdafx.h"
#include "util-niscope.h"
#include "microscope.h"
#include "device-digitizer.h"
#include "device.h"

#define CheckWarn( expression )  (niscope_chk( g_digitizer.vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( g_digitizer.vi, expression, #expression, &error   ))
#define ViErrChk( expression )    goto_if_fail( VI_SUCCESS == CheckWarn(expression), Error )

Digitizer             g_digitizer              = DIGITIZER_DEFUALT;
Device               *gp_digitizer_device      = NULL;

unsigned int Digitizer_Destroy(void)
{ if( !Device_Release( gp_digitizer_device, DIGITIZER_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly release digitizer.\r\n");
  Device_Free( gp_digitizer_device );
  return 0;
}

void Digitizer_Init(void)
{ Guarded_Assert( gp_digitizer_device = Device_Alloc() );
  gp_digitizer_device->context = (void*) &g_digitizer;

  // Register Shutdown functions - these get called in reverse order
  Register_New_Shutdown_Callback( &Digitizer_Destroy );
  Register_New_Shutdown_Callback( &Digitizer_Close   );
  
  // Register Microscope state functions
  Register_New_Microscope_Hold_Callback( &Digitizer_Hold );
  Register_New_Microscope_Off_Callback( &Digitizer_Off );
}

unsigned int Digitizer_Close(void)
{ ViStatus status = 1; //error
  Device_Lock( gp_digitizer_device );
  debug("Digitizer: Attempting to close vi: %d\r\n", g_digitizer.vi);
  if( !Device_Release( gp_digitizer_device, DIGITIZER_DEFAULT_TIMEOUT ) )
    warning("Could not cleanly release digitizer.\r\n");
  { ViSession vi = g_digitizer.vi;
    if(vi)
      ViErrChk( niScope_close(vi) );  // Close the session
  }
  status = 0;  // success
  debug("Digitizer: Closed.\r\n");
Error:  
  g_digitizer.vi = 0;
  Device_Unlock( gp_digitizer_device );
  return status;
}

unsigned int Digitizer_Off(void)
{ debug("Digitizer: Off\r\n");
  return( Digitizer_Close() );
}

unsigned int Digitizer_Hold(void)
{ Device_Lock( gp_digitizer_device );
  ViStatus status = VI_SUCCESS;
  debug("Digitizer: Hold\r\n");
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
  debug("\tGot session %3d with status %d\n",g_digitizer.vi,status);
  Device_Unlock( gp_digitizer_device );

  return status;
}

//
// TASK - Streaming on all channels
//

unsigned int
_Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Cfg( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ ViSession  vi = g_digitizer.vi;
  Digitizer_Config cfg = g_digitizer.config;
  ViInt32 i;
   
  // Vertical
  for(i=0; i<cfg.num_channels; i++)
  { Digitizer_Channel_Config ch = cfg.channels[i];
    CheckPanic(niScope_ConfigureVertical (vi, 
                                          ch.name,     //channelName, 
                                          ch.range,    //verticalRange, 
                                          0.0,         //verticalOffset, 
                                          ch.coupling, //verticalCoupling, 
                                          1.0,         //probeAttenuation, 
                                          ch.enabled));//enabled?
  }
  // Horizontal
  CheckPanic (niScope_ConfigureHorizontalTiming (vi, 
                                                cfg.sample_rate,       // sample rate (S/s)
                                                cfg.record_length,     // record length (S)
                                                cfg.reference_position,// reference position (% units???)
                                                cfg.num_records,       // number of records to fetch per acquire
                                                NISCOPE_VAL_TRUE));    // enforce real time?
  // Configure software trigger, but never send the trigger.
  // This starts an infinite acquisition, until you call niScope_Abort
  // or niScope_close
  CheckPanic (niScope_ConfigureTriggerSoftware (vi, 
                                                 0.0,   // hold off (s)
                                                 0.0)); // delay    (s)
  
  { ViInt32 nwfm;   
    size_t nbuf[2] = {32,32},
             sz[2] = {1024*1024*sizeof(ViReal64*), nwfm*sizeof(struct niScope_wfmInfo)};
    CheckPanic( niScope_ActualNumWfms(vi, cfg.acquisition_channels, &nwfm ) );
    DeviceTask_Configure_Outputs( d->task, 2, nbuf, sz );
  }
  return 1;
}

unsigned int
_Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Proc( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ Digitizer *dig = ((Digitizer*)d->context);
  ViSession   vi = dig->vi;
  ViChar   *chan = dig->config.acquisition_channels;
  asynq   *qdata = out->contents[0],
          *qwfm  = out->contents[1];
  ViInt32  nelem = qdata->q->buffer_size_bytes / sizeof(ViReal64),          // number of samples per data buffer
           ttl = 0,
           old_state;
  ViReal64               *buf = (ViReal64*)        Asynq_Token_Buffer_Alloc(qdata);
  struct niScope_wfmInfo *wfm = (niScope_wfmInfo*) Asynq_Token_Buffer_Alloc(qwfm);

  ViErrChk (niScope_InitiateAcquisition (vi));
  ViErrChk   (niScope_GetAttributeViInt32 (vi, NULL,   // TODO: reset to default when done
                                           NISCOPE_ATTR_FETCH_RELATIVE_TO,         //?TODO: push/pop state for niscope?
                                           &old_state ));    
  ViErrChk   (niScope_SetAttributeViInt32 (vi, NULL,   // TODO: reset to default when done
                                           NISCOPE_ATTR_FETCH_RELATIVE_TO,         //?TODO: push/pop state for niscope?
                                           NISCOPE_VAL_READ_POINTER ));
  // Loop until the stop event is triggered
  do 
  {
     // Fetch the available data without waiting
     ViErrChk (niScope_Fetch (vi, 
                              chan,          // (acquistion channels)
                              0.0,           // Immediate
                              nelem - ttl,   // Remaining space in buffer
                              buf   + ttl,   // Where to put the data
                              wfm));         // metadata for fetch
     Asynq_Push( qwfm,(void**) &wfm, 0 );    // Push the info from the last fetch
     ttl += wfm->actualSamples;      // add the chunk size to the total samples count
     if( ttl == nelem )              // Is buffer full?
     { Asynq_Push( qdata,(void**) &buf, 0 ); //   Push buffer and reset total samples count
       ttl = 0;
     }       
  } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
  
  CheckPanic( niScope_Abort(vi) );
  return 1;  
Error:  
  CheckPanic( niScope_Abort(vi) );
  return 0;
}

DeviceTask*
Digitizer_Create_Task_Stream_All_Channels_Immediate_Trigger(void)
{ return DeviceTask_Alloc(_Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Cfg,
                          _Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Proc);
}


//
// USER INTERFACE (WINDOWS)
//

typedef struct _digitizer_ui_main_menu_state
{ HMENU menu,
        taskmenu;

} digitizer_ui_main_menu_state;

digitizer_ui_main_menu_state g_digitizer_ui_main_menu_state = {0};

void Digitizer_UI_Append_Menu( HMENU menu )
{ HMENU submenu = CreatePopupMenu(),
        taskmenu = CreatePopupMenu();
  Guarded_Assert_WinErr( AppendMenu( taskmenu, MF_STRING, IDM_DIGITIZER_TASK_STREAM1, "Continuous &Fetch" ));
  
                                       
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_OFF,  "&Off"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_HOLD, "&Hold"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING | MF_POPUP,  (UINT_PTR) taskmenu, "&Tasks"));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_TASK_STOP, "&Run" ));
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_TASK_STOP, "&Stop" ));
  
  Guarded_Assert_WinErr( AppendMenu( submenu, MF_STRING, IDM_DIGITIZER_LIST_DEVICES, "&List NI modular devices"));
  
  Guarded_Assert_WinErr( AppendMenu(    menu, MF_STRING | MF_POPUP, (UINT_PTR) submenu, "&Digitizer"));
  
  g_digitizer_ui_main_menu_state.menu     = submenu;
  g_digitizer_ui_main_menu_state.taskmenu = taskmenu;
}

// This has the type of a WinProc
// Returns:
//    0  if the message was handled
//    1  otherwise
LRESULT CALLBACK Digitizer_UI_Handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_INITMENU:
	case WM_INITMENUPOPUP:
	  { HMENU hmenu = (HMENU) wParam;
	    char str[1024] = "\0";
	    Guarded_Assert_WinErr( GetMenuString( hmenu, 0, str, 1024, MF_BYPOSITION) );
	    debug("is digitizer      menu: %s\r\n"
	          "is digitizer task menu: %s\r\n",
	          (hmenu==g_digitizer_ui_main_menu_state.menu)?"yes":"no",
	          (hmenu==g_digitizer_ui_main_menu_state.taskmenu)?"yes":"no"
	         );
	    if( hmenu == g_digitizer_ui_main_menu_state.menu )
	    { Guarded_Assert_WinErr(-1!=                                // Grey out the tasks menu
	        EnableMenuItem( hmenu, 2 /*Tasks*/, MF_BYPOSITION       //   if the vi session is null
	                        | ( (g_digitizer.vi==0)?MF_GRAYED:MF_ENABLED) ));
        Guarded_Assert_WinErr(-1!=                               // Grey out the tasks menu
	        EnableMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION        //   if the vi session is null
	                        | ( (gp_digitizer_device->is_available==0
	                             && gp_digitizer_device->task != NULL)?MF_ENABLED:MF_GRAYED) ));
        Guarded_Assert_WinErr(-1!=                               // Grey out the tasks menu
	        EnableMenuItem( hmenu, 4 /*Stop*/, MF_BYPOSITION       //   if the vi session is null
	                        | ( (gp_digitizer_device->is_available==0)?MF_GRAYED:MF_ENABLED) ));	                        
	      Guarded_Assert(-1!=                                                   // Detect state
	        CheckMenuRadioItem(g_digitizer_ui_main_menu_state.menu,             // Toggle radio button to reflect
                             0,1,((g_digitizer.vi==0)?0:1), MF_BYPOSITION ) );
	    } else if( hmenu == g_digitizer_ui_main_menu_state.taskmenu )
	    { Guarded_Assert_WinErr(-1!=                      // Grey out the tasks menu
	        EnableMenuItem( hmenu, 0, MF_BYPOSITION       //   if the vi session is null
	                        | ( (gp_digitizer_device->task==0)?MF_GRAYED:MF_ENABLED) ));
	    }
	    return 1;
	  }
	  break;
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
    case IDM_DIGITIZER_LIST_DEVICES:
      { debug("IDM_DIGITIZER_LIST_DEVICES\r\n");
        niscope_debug_list_devices();        
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
