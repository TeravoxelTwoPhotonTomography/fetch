
//
// TASK 
//  - synchronize on analog edge from AI0.  Should be connected to resonant
//    mirror's velocity signal.  Triggers on a rising zero crossing.
//  - on ai0 trigger,
//    - generate a pulse on ctr0...this is the frame sync
//      ctr0 will be output to the digitizer's PFI0 and used as the acquisition
//        start trigger.  See device-task-digitizer-triggered.cpp
//    - generate a saw-tooth or triangle wave (haven't decided yet) on ao0
//      ao0 should be connected to galvo scan mirror.
//

// [ ] How to set up a digital pulse via a counter (example?)
// [ ] How to get the trigger source from the device? (extrapolate from vi)

int32 CVICALLBACK
_Galvo_Mirror_Task_Triggered_Done_Callback(TaskHandle taskHandle, int32 status, void *callbackData)
{ OnErrPanic(status);

  SetEvent( gp_galvo_mirror_device->notify_stop );
  return 0;
}

TaskHandle
_Galvo_Mirror_Task_Triggered__Create_Frame_Sync(void)
{ TaskHandle task;
  OnErrPanic( DAQmxCreateTask( "",                           // task name
                              &task ));                      // task handle
                              
  OnErrPanic( DAQmxCreateCOPulseChanTime( task,                 // task handle
                                          "Dev1\co0",           // counter name
                                          "",                   // asigned name
                                          DAQmx_Val_Seconds,    // units (time)
                                          DAQmx_Val_Low,        // idle state
                                          0.0,                  // initial delay
                                          0.010,                // low  time (s)
                                          0.010 ));             // high time (s)
                                          
  OnErrPanic( DAQmxCfgDigEdgeStartTrig( task,                   // task handle
                                        "ao/StartTrigger", // trigger source
                                        DAQmx_Val_Rising ));    // edge
 
  // Set retriggerable - will repeat frame sync on next resonant velocity zero crossing
  OnErrPanic( DAQmxSetStartTrigRetriggerable( task, VI_TRUE ));
  
  // Use the Arm-Start trigger to gate next frame sync so that it waits till the 
  //     digitizer is ready.
  OnErrPanic( DAQmxSetArmStartTrigType        ( task, DAQmx_Val_DigEdge ));
  OnErrPanic( DAQmxSetDigEdgeArmStartTrigSrc  ( task, "RTSI0" );
  OnErrPanic( DAQmxSetDigEdgeArmStartTrigEdge ( task, DAQmx_Val_Rising );

                                          
}

unsigned int
_Galvo_Mirror_Task_Triggered_Cfg( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ TaskHandle           dc = g_galvo_mirror.task_handle;
  Galvo_Mirror_Config cfg = g_galvo_mirror.config;  
   
  // Vertical
  OnErrPanic( DAQmxCreateAOVoltageChan( dc, 
                                        cfg.physical_channel,
                                        cfg.channel_label,
                                        cfg.min,
                                        cfg.max,
                                        cfg.units,
                                        NULL ));
  // Horizontal
  OnErrPanic( DAQmxCfgSampClkTiming(    dc,
                                        cfg.source,
                                        cfg.rate,
                                        cfg.active_edge,
                                        cfg.sample_mode,
                                        cfg.samples_per_channel ));
  
  // Event callbacks
  OnErrPanic( 
    DAQmxRegisterDoneEvent( 
      dc, 
      0, /*use the daqmx thread for the callback's execution*/
      _Galvo_Mirror_Task_Triggered_Done_Callback, 
      NULL ));
  
  // Configure input queue
  { size_t nbuf = 2,
             sz = ((size_t)cfg.rate)*sizeof(float64);
    DeviceTask_Alloc_Inputs( d->task, 1, &nbuf, &sz );
  }
  
  // Prime the ADC
  { asynq   *q   = d->task->in->contents[0];
    int32    nout,
             n   = (int32)q->q->buffer_size_bytes / sizeof(float64); // number of samples per data buffer
    float64 *buf = (float64*) Asynq_Token_Buffer_Alloc(q);
        
    //memset(buf,0,q->q->buffer_size_bytes);
    
    // Make a triangle wave
    { float64                  A = (cfg.max - cfg.min)/2.0 - 1e-3,
              resonant_frequency = 7920, // Hz
                 lines_per_frame = 512,
                 frame_frequency = 2.0*resonant_frequency/lines_per_frame,   // 1. Line rate is twice resonant frequency                 
              k = q->q->buffer_size_bytes/sizeof(float64)/frame_frequency/2; // 2. Two frames per full wave
      size_t i,n = q->q->buffer_size_bytes/sizeof(float64);
      // first pass - sawtooth - one full rise is 2 frames
      i=n;
      while(i--)
      { float64 t = i/k;
        buf[i] = t - floor(t);
      }
      
      { FILE* fp = fopen("galvo-wave-first-pass-f64.raw","wb");
        fwrite(buf,1,q->q->buffer_size_bytes,fp);
        fclose(fp);
      }      
      
      // second pass - transform sawtooth to triangle with appropriate amplitude
      //             - t==0 => full amplitude
      i=n;      
      while(i--)
      { float64 v = buf[i] - 0.5;
        buf[i] = A*(2.0*v*SIGN(v)-0.5);
      }
      
      { FILE* fp = fopen("galvo-wave-second-pass-f64.raw","wb");
        fwrite(buf,1,q->q->buffer_size_bytes,fp);
        fclose(fp);
      }
    }    

    OnErrPanic( DAQmxWriteAnalogF64(dc, /* task handle */
                                    (int32) cfg.samples_per_channel, 
                                    0,  /*autostart*/ 
                                    0,  /*timeout  */
                                    DAQmx_Val_GroupByChannel,  /*interleaved*/
                                    buf,     /*data to write*/                                    
                                    &nout,   /* samples_written*/
                                    NULL) ); /*reserved*/
    debug("Scan mirror: %5d samples written.\r\n");
    Asynq_Push(q,(void**)&buf,FALSE);
    Asynq_Token_Buffer_Free(buf);
  }                                  
  debug("Galvo Mirror configured for continuous-scan-immediate-trigger\r\n");
  return 1;
}

unsigned int
_Galvo_Mirror_Task_Triggered_Proc( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ Galvo_Mirror *m = ((Galvo_Mirror*)d->context);
  TaskHandle dc = m->task_handle;
  unsigned int ret = 1; // success

  OnErrPanic (DAQmxStartTask(dc));
  // Wait for stop event is triggered
  debug("Galvo_Mirror Fetch_Forever - Running -\r\n");
  
  if( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, INFINITE) )
    warning("Wait for continuous scan stopped prematurely\r\n");     
  OnErrPanic( DAQmxStopTask(dc) );
  //OnErrPanic( DAQmxWriteAnalogScalarF64(dc,0,0,0.0,NULL) );

  debug("Galvo_Mirror Fetch_Forever - Running done-\r\n"
        "Task done: normal exit\r\n");
        
  ret = 0; //success
  return ret;
}

DeviceTask*
Galvo_Mirror_Create_Task_Triggered(void)
{ return DeviceTask_Alloc(_Galvo_Mirror_Task_Triggered_Cfg,
                          _Galvo_Mirror_Task_Triggered_Proc);
}

extern inline DeviceTask*
Galvo_Mirror_Get_Default_Task(void)
{ return gp_galvo_mirror_tasks[0];
}
