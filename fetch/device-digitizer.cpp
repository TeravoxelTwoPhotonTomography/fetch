#include "stdafx.h"
#include "util-niscope.h"
#include "microscope.h"
#include "device-digitizer.h"
#include "device.h"
#include "frame.h"
#include "frame-interface-digitizer.h"

// Operational options
#if 0
#define DIGITIZER_NO_REGISTER_WITH_MICROSCOPE
#endif

// debug defines

#if 1
#define DIGITIZER_DEBUG_FAIL_WHEN_FULL
#else
#define DIGITIZER_DEBUG_SPIN_WHEN_FULL
#endif

#define CheckWarn( expression )  (niscope_chk( g_digitizer.vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( g_digitizer.vi, expression, #expression, &error   ))
#define ViErrChk( expression )    goto_if( CheckWarn(expression), Error )

typedef ViInt16 TPixel;

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
  return 0;
}

void Digitizer_Init(void)
{ Guarded_Assert( gp_digitizer_device = Device_Alloc() );
  gp_digitizer_device->context = (void*) &g_digitizer;

  // Register Shutdown functions - these get called in reverse order
  Register_New_Shutdown_Callback( &_digitizer_free_tasks );
  Register_New_Shutdown_Callback( &Digitizer_Destroy );
  Register_New_Shutdown_Callback( &Digitizer_Detach );


#ifndef DIGITIZER_NO_REGISTER_WITH_MICROSCOPE  
  // Register Microscope state functions
  Register_New_Microscope_Attach_Callback( &Digitizer_Attach );
  Register_New_Microscope_Detach_Callback( &Digitizer_Detach );
#endif
  
  // Create tasks
  gp_digitizer_tasks[0] = Digitizer_Create_Task_Stream_All_Channels_Immediate_Trigger();
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

Device*
Digitizer_Get_Device(void)
{ return gp_digitizer_device;
}

//
// TASK - Streaming on all channels
//

Digitizer_Frame_Metadata*
_Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Frame_Metadata( ViInt32 record_length, ViInt32 nwfm )
{ static Digitizer_Frame_Metadata meta;
  
    
  meta.height = 512;
  meta.width  = (u16) (record_length / meta.height);
  meta.nchan  = (u8)  nwfm;
  meta.Bpp    = sizeof(TPixel);
  return &meta;
}

unsigned int
_Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Cfg( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ ViSession  vi = g_digitizer.vi;
  Digitizer_Config cfg = g_digitizer.config;
  ViInt32 i;
  int nchan = 0;
   
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
    nchan += ch.enabled;
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
    ViInt32 record_length;
    Frame_Descriptor desc;
    
    CheckPanic( niScope_ActualNumWfms(vi, cfg.acquisition_channels, &nwfm ) );
    CheckPanic( niScope_ActualRecordLength(vi, &record_length) );
    
    // Fill in frame description
    { Digitizer_Frame_Metadata *meta = 
        _Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Frame_Metadata( record_length, nwfm );
      Frame_Descriptor_Change( &desc, FRAME_INTEFACE_DIGITIZER__INTERFACE_ID, meta, sizeof(Digitizer_Frame_Metadata) );
    }
    { size_t nbuf[2] = {DIGITIZER_BUFFER_NUM_FRAMES,
                        DIGITIZER_BUFFER_NUM_FRAMES},
               sz[2] = {Frame_Get_Size_Bytes(&desc),             // frame data
                        nwfm*sizeof(struct niScope_wfmInfo)};    // description of each frame
      // DeviceTask_Free_Outputs( d->task );  // free channels that already exist (FIXME: thrashing)
      // Channels are reference counted so the memory may not be freed till the other side Unrefs.
      if( d->task->out == NULL ) // channels may already be alloced (occurs on a detach->attach cycle)
        DeviceTask_Alloc_Outputs( d->task, 2, nbuf, sz );
    }
  }
  debug("Digitizer configured for Stream_All_Channels_Immediate_Trigger\r\n");
  return 1;
}

unsigned int
_Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Proc( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ Digitizer *dig = ((Digitizer*)d->context);
  ViSession   vi = dig->vi;
  ViChar   *chan = dig->config.acquisition_channels;
  asynq   *qdata = out->contents[0],
          *qwfm  = out->contents[1];
  ViInt32  nelem,
           nwfm,
           ttl = 0,ttl2=0,
           old_state;
  u32      nfetches = 0,
           nframes  = 0,
           every    = 32;  // must be power of two - used for reporting FPS
  TicTocTimer t, delay_clock;
  double delay, maxdelay = 0.0, accdelay = 0.0;
  u32    last_max_fetch = 0;  
  Frame                  *frm  = (Frame*)            Asynq_Token_Buffer_Alloc(qdata);
  struct niScope_wfmInfo *wfm  = (niScope_wfmInfo*)  Asynq_Token_Buffer_Alloc(qwfm);
  Frame_Descriptor       *desc;
  TPixel                 *buf;
  unsigned int ret = 1;

  Frame_From_Bytes(frm, (void**)&buf, &desc );
    
  CheckPanic( niScope_ActualNumWfms(vi, chan, &nwfm ) );
  CheckPanic( niScope_ActualRecordLength(vi, &nelem) );

  // Fill in frame description
  { Digitizer_Frame_Metadata *meta = 
      _Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Frame_Metadata( nelem, nwfm );
    Frame_Descriptor_Change( desc, FRAME_INTEFACE_DIGITIZER__INTERFACE_ID, meta, sizeof(Digitizer_Frame_Metadata) );
  }
      
  
  ViErrChk   (niScope_GetAttributeViInt32 (vi, NULL,   // TODO: reset to default when done
                                           NISCOPE_ATTR_FETCH_RELATIVE_TO,         //?TODO: push/pop state for niscope?
                                           &old_state ));    
  ViErrChk   (niScope_SetAttributeViInt32 (vi, NULL,   // TODO: reset to default when done
                                           NISCOPE_ATTR_FETCH_RELATIVE_TO,         //?TODO: push/pop state for niscope?
                                           NISCOPE_VAL_READ_POINTER ));

  // Loop until the stop event is triggered
  debug("Digitizer Stream_All_Channels_Immediate_Trigger - Running -\r\n");
  t = tic();
  
  ViErrChk   (niScope_InitiateAcquisition (vi));
  delay_clock = tic();
  do 
  { ttl2 += ttl;
    ttl = 0;
    do
    { // Fetch the available data without waiting
      delay = toc( &delay_clock );
      delay = toc( &delay_clock );
      maxdelay = MAX(delay, maxdelay);
      ViStatus sts = niScope_FetchBinary16 ( vi, 
                                        chan,          // (acquistion channels)
                                        0.0,           // Immediate
                                        nelem - ttl,   // Remaining space in buffer
                                        buf   + ttl,   // Where to put the data
                                        wfm);          // metadata for fetch
      if( delay > maxdelay )
      { maxdelay = delay;
        last_max_fetch = nfetches;
      }
      accdelay += delay;
      if( sts != VI_SUCCESS )
      { niscope_chk(vi,sts, "niScope_FetchBinary16", warning);
        goto Error;
      }
      ++nfetches;     
      ttl += wfm->actualSamples;  // add the chunk size to the total samples count     
      Asynq_Push( qwfm,(void**) &wfm, 0 );    // Push (swap) the info from the last fetch
    } while(ttl!=nelem);
    
    // Handle the full buffer
    { double dt;
#ifdef DIGITIZER_DEBUG_FAIL_WHEN_FULL
      if(  !Asynq_Push_Try( qdata,(void**) &frm )) //   Push buffer and reset total samples count
#elif defined( DIGITIZER_DEBUG_SPIN_WHEN_FULL )
      if(  !Asynq_Push( qdata,(void**) &frm, FALSE )) //   Push buffer and reset total samples count
#else
        error("Choose a push behavior for digitizer by compileing with the appropriate define.\r\n");
#endif
      { warning("Digitizer output queue overflowed.\r\n\tAborting acquisition task.\r\n");
        goto Error;
      }
      ++nframes;
      { ViReal64 pts = 0;
        CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_BACKLOG, &pts ));
        debug("Digitizer Backlog: %4.1f MS\r\n",pts/1024.0/1024.0);
      }  
      Frame_From_Bytes(frm, (void**)&buf, &desc ); //get addresses 
      desc->is_change = 0; // Mark future frame descriptors as unchanged from the last.
      
      //dt = toc(&t);
      //if( !MOD_UNSIGNED_POW2(nframes+1,every) )
      //  debug("FPS: %3.1f Frame time: %5.4f MS/s: %3.1f  MB/s: %3.1f Q: %3d Digitizer\r\n",
      //        1.0/dt, dt, nelem/dt/1000000.0, nelem*sizeof(TPixel)*nwfm/1000000.0/dt,
      //        qdata->q->head - qdata->q->tail );
    }    
  } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
  debug("Digitizer Stream_All_Channels_Immediate_Trigger - Running done -\r\n"
        "Task done: normal exit\r\n");
  ret = 0; //success
Error:
  free( frm );
  free( wfm );
  debug("Digitizer: nfetches: %u nframes: %u\r\n"
        "\tDelay - max: %g (on fetch %d) mean:%g\r\n"
        "\tTotal acquired samples %f MS\r\n",nfetches, nframes,maxdelay, last_max_fetch,accdelay/nfetches,ttl2/1024.0/1024.0);
  { ViReal64 pts = 0;
    CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_BACKLOG, &pts ));
    debug("Digitizer Backlog: %4.1f MS\r\n",pts/1024.0/1024.0);
  }
  { ViInt32 mem = 0;
    CheckPanic( niScope_GetAttributeViInt32( vi, NULL, NISCOPE_ATTR_ONBOARD_MEMORY_SIZE, &mem ));
    debug("Digitizer                          Buffer size: %4.1f MB\r\n",mem/1024.0/1024.0);
  }
  { ViInt32 mem = 0;
    CheckPanic( niScope_GetAttributeViInt32( vi, NULL, NISCOPE_ATTR_DATA_TRANSFER_BLOCK_SIZE, &mem ));
    debug("Digitizer             Data Transfer Block size: %4.1f MB\r\n",mem/1024.0/1024.0);
  }
  { ViReal64 mem = 0;
    CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_DATA_TRANSFER_MAXIMUM_BANDWIDTH, &mem ));
    debug("Digitizer Data Transfer Maximum Bandwidth size: %4.1f MB\r\n",mem/1024.0/1024.0);
  }
  { ViInt32 mem = 0;
    CheckPanic( niScope_GetAttributeViInt32( vi, NULL, NISCOPE_ATTR_DATA_TRANSFER_PREFERRED_PACKET_SIZE, &mem ));
    debug("Digitizer   Data Transfer Prefered Packet size: %4.1f MB\r\n",mem/1024.0/1024.0);
  }
  { ViReal64 mem = 0;
    CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_MAX_REAL_TIME_SAMPLING_RATE, &mem ));
    debug("Digitizer     Data Max real time sampling rate: %4.1f MHz\r\n",mem/1024.0/1024.0);
  }
  { ViReal64 mem = 0;
    CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_HORZ_SAMPLE_RATE, &mem ));
    debug("Digitizer                 actual sampling rate: %4.1f MHz\r\n",mem/1024.0/1024.0);
  }
  { ViReal64 mem = 0;
    CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_DEVICE_TEMPERATURE, &mem ));
    debug("Digitizer                          Temperature: %4.1f C\r\n",mem);
  }
  CheckPanic( niScope_Abort(vi) );
  return ret;
}

DeviceTask*
Digitizer_Create_Task_Stream_All_Channels_Immediate_Trigger(void)
{ return DeviceTask_Alloc(_Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Cfg,
                          _Digitizer_Task_Stream_All_Channels_Immediate_Trigger_Proc);
}

DeviceTask*
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
  Guarded_Assert_WinErr( AppendMenu( taskmenu, MF_STRING, IDM_DIGITIZER_TASK_0, "Continuous &Fetch" ));
  
                                       
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
	                        | ( Device_Is_Armed(gp_digitizer_device)?MF_CHECKED:MF_UNCHECKED) ));
        Guarded_Assert_WinErr(-1!=
	        EnableMenuItem( hmenu, 3 /*Run*/, MF_BYPOSITION
	                        | ( (Device_Is_Armed(gp_digitizer_device))?MF_ENABLED:MF_GRAYED) ));
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
	      if( Device_Is_Armed( gp_digitizer_device ) || Device_Is_Running( gp_digitizer_device ) )
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
