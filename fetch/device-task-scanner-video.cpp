
#include "stdafx.h"
#include "util-niscope.h"
#include "util-nidaqmx.h"
#include "device-digitizer.h"
#include "frame.h"
#include "frame-interface-digitizer.h"
#include "device.h"
#include "device-scanner.h"
#include "device-task-scanner-video.h"

#define SCANNER_VIDEO_TASK_FETCH_TIMEOUT  100.0 //10.0, //(-1=infinite) (0.0=immediate)

#if 1
#define scanner_debug(...) debug(__VA_ARGS__)
#else
#define scanner_debug(...)
#endif

#define DIGWRN( expr )  (niscope_chk( Scanner_Get()->digitizer->vi, expr, #expr, warning ))
#define DIGERR( expr )  (niscope_chk( Scanner_Get()->digitizer->vi, expr, #expr, error   ))
#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )  (goto_if_fail( 0==DAQWRN(expr), Error))

#if 1
#define SCANNER_DEBUG_FAIL_WHEN_FULL
#else
#define SCANNER_DEBUG_SPIN_WHEN_FULL
#endif

typedef ViInt16 TPixel;

//----------------------------------------------------------------------------
//
//  Video frame configuration
//
Digitizer_Frame_Metadata*
_Scanner_Task_Video_Metadata( ViInt32 record_length, ViInt32 nwfm )
{ static Digitizer_Frame_Metadata meta;    
  meta.height = Scanner_Get()->config.scans; //512;
  meta.nchan  = (u8)  (nwfm / meta.height);
  meta.width  = (u16) (record_length / meta.nchan);
  meta.Bpp    = sizeof(TPixel);
  Guarded_Assert( meta.nchan  > 0 );
  Guarded_Assert( meta.height > 0 );
  Guarded_Assert( meta.width  > 0 );
  return &meta;
}

//----------------------------------------------------------------------------
//
//  Configuration
//

ViInt32
_compute_record_size(void)
{ Scanner          *scanner = Scanner_Get();
  Scanner_Config   *scn_cfg = &scanner->config;
  Digitizer_Config *dig_cfg = &scanner->digitizer->config;
  return (ViInt32) (scn_cfg->line_duty_cycle * dig_cfg->sample_rate / scn_cfg->resonant_frequency);
}

// Acquisition
void _config_digitizer( void )
{ Scanner          *scanner = Scanner_Get();
  Scanner_Config   *scn_cfg = &scanner->config;
  Digitizer_Config *dig_cfg = &scanner->digitizer->config;
  ViSession              vi = scanner->digitizer->vi;

  Digitizer_Channel_Config
           line_trigger_cfg = dig_cfg->channels[ scn_cfg->line_trigger_src ];

  ViReal64   refPosition         = 0.0;
  ViReal64   verticalOffset      = 0.0;
  ViReal64   probeAttenuation    = 1.0;
  ViBoolean  enforceRealtime     = NISCOPE_VAL_TRUE;

  Guarded_Assert( dig_cfg->channels[ scn_cfg->line_trigger_src ].enabled );

  niscope_cfg_rtsi_default( vi );
  DIGERR( niScope_ConfigureVertical(vi,
                                    line_trigger_cfg.name,       // channelName
                                    line_trigger_cfg.range,
                                    0.0,
                                    line_trigger_cfg.coupling,
                                    probeAttenuation,
                                    NISCOPE_VAL_TRUE));          // ?? what's this
  DIGERR( niScope_ConfigureHorizontalTiming(vi,
                                            dig_cfg->sample_rate,
                                            _compute_record_size(),
                                            refPosition,
                                            scn_cfg->scans,
                                            enforceRealtime));

  // Analog trigger for bidirectional scans
  DIGERR( niScope_ConfigureTriggerEdge (vi,
                                        line_trigger_cfg.name,     // channelName
                                        0.0,                       // triggerLevel
                                        NISCOPE_VAL_POSITIVE,      // triggerSlope
                                        line_trigger_cfg.coupling, // triggerCoupling
                                        0.0,                       // triggerHoldoff
                                        0.0 ));                    // triggerDelay
  // Wait for start trigger (frame sync) on PFI1
  DIGERR( niScope_SetAttributeViString( vi,
                                        "",
                                        NISCOPE_ATTR_ACQ_ARM_SOURCE,
                                        NISCOPE_VAL_PFI_1 ));
}

// Configure DAQ for the frame program
// - slow mirror scan
// - frame sync 
void _config_daq(void)
{ Scanner     *scanner = Scanner_Get();
  Scanner_Config  *cfg = &scanner->config;
  TaskHandle  cur_task = 0;
  float64        *data = NULL;
  int32        written;

  ViInt32   N          = cfg->galvo_samples;
  float64   frame_time = cfg->scans / cfg->resonant_frequency; //  512 records / (7920 records/sec)
  float64   freq       = N/frame_time;                         // 4096 samples / 64 ms = 63 kS/s

  data = (float64*) Guarded_Malloc( sizeof(float64) * N, "scanner::_config_daq" );

  // sawtooth
  { int i=N;
    float64 A = cfg->galvo_vpp;
    while(i--)
      data[i] = A*((double)i/((double)N)-0.5);    // linear ramp from -A/2 to A/2
  }

  // set up ao task
  cur_task = scanner->daq_ao;
  DAQERR( DAQmxCreateAOVoltageChan   (cur_task,
                                      cfg->galvo_channel,   //eg: "/Dev1/ao0"
                                      "galvo-mirror-out",   //name to assign to channel
                                      cfg->galvo_v_lim_min, //Volts eg: -10.0
                                      cfg->galvo_v_lim_max, //Volts eg:  10.0
                                      DAQmx_Val_Volts,      //Units
                                      NULL));               //Custom scale (none)
  DAQERR( DAQmxCfgSampClkTiming      (cur_task,
                                      cfg->galvo_clock,     // "Ctr1InternalOutput",
                                      freq,
                                      DAQmx_Val_Rising,
                                      DAQmx_Val_ContSamps,  // use continuous output so that counter stays in control
                                      N));
  DAQERR( DAQmxCfgAnlgEdgeStartTrig  (cur_task,
                                      cfg->galvo_trigger,
                                      DAQmx_Val_Rising,
                                      0.0));
  DAQERR( DAQmxWriteAnalogF64        (cur_task,
                                      N,
                                      0,                           // autostart?
                                      10.0,                        // timeout (s) - to write?
                                      DAQmx_Val_GroupByChannel,
                                      data,
                                      &written,
                                      NULL));
  Guarded_Assert( written == N );
  free(data);

  // set up counter for sample clock
  cur_task = scanner->daq_clk;
  DAQERR( DAQmxCreateCOPulseChanFreq       ( cur_task,
                                             cfg->galvo_ctr, // "Dev1/ctr1",
                                             "sample-clock", 
                                             DAQmx_Val_Hz, 
                                             DAQmx_Val_Low,
                                             0.0,
                                             freq,
                                             0.5 ));
  DAQERR( DAQmxCfgImplicitTiming           ( cur_task, DAQmx_Val_FiniteSamps, N ));
  DAQERR( DAQmxCfgDigEdgeStartTrig         ( cur_task, "AnalogComparisonEvent", DAQmx_Val_Rising ));
  DAQERR( DAQmxSetArmStartTrigType         ( cur_task, DAQmx_Val_DigEdge ));
  DAQERR( DAQmxSetDigEdgeArmStartTrigSrc   ( cur_task, cfg->galvo_armstart ));
  DAQERR( DAQmxSetDigEdgeArmStartTrigEdge  ( cur_task, DAQmx_Val_Rising ));
  return;
}

void
_fill_frame_description( Frame_Descriptor *desc )
{ ViSession                   vi = Scanner_Get()->digitizer->vi;
  ViInt32                   nwfm;
  ViInt32          record_length;
  Digitizer_Frame_Metadata *meta = NULL;

  DIGERR( niScope_ActualNumWfms(vi, 
                                Scanner_Get()->digitizer->config.acquisition_channels,
                                &nwfm ) );
  DIGERR( niScope_ActualRecordLength(vi, &record_length) );

  meta = _Scanner_Task_Video_Metadata( record_length, nwfm );
  Frame_Descriptor_Change( desc,
                           FRAME_INTERFACE_DIGITIZER_INTERLEAVED_PLANES__INTERFACE_ID,
                           meta,
                           sizeof( Digitizer_Frame_Metadata ) );
}

unsigned int
_Scanner_Task_Video_Cfg( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )              // this whole thing is super ugly
{ _config_daq();
  _config_digitizer();

  // Allocate output pipes
  { ViInt32                   nwfm;
    Frame_Descriptor          desc;
    ViSession                   vi = Scanner_Get()->digitizer->vi;
    char                     *chan = Scanner_Get()->digitizer->config.acquisition_channels;
    
    DIGERR( niScope_ActualNumWfms( vi,chan,&nwfm ) );    
    _fill_frame_description(&desc);
    { size_t               nbuf[2] = { SCANNER_QUEUE_NUM_FRAMES,
                                       SCANNER_QUEUE_NUM_FRAMES },
                             sz[2] = { Frame_Get_Size_Bytes( &desc ),
                                       nwfm*sizeof(struct niScope_wfmInfo) };
      if( d->task->out == NULL )
        DeviceTask_Alloc_Outputs( d->task, 2, nbuf, sz );
    }
  }
  debug("Scanner configured for Video\r\n");
  return 1; //success
}

//----------------------------------------------------------------------------
//
//  Proc
//

unsigned int
_Scanner_Task_Video_Proc( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ asynq *qdata = out->contents[0],
        *qwfm  = out->contents[1];
  Frame                  *frm = (Frame*) Asynq_Token_Buffer_Alloc(qdata);
  struct niScope_wfmInfo *wfm = (niScope_wfmInfo*) Asynq_Token_Buffer_Alloc(qwfm);
  TPixel *buf;
  Frame_Descriptor *desc, ref;
  int change_token;
  int width = _compute_record_size();
  int i=0, status = 1; // status == 0 implies success, error otherwise
  TicTocTimer outer_clock = tic(),
              inner_clock = tic();
  double dt_in=0.0, dt_out=0.0;
  
  Scanner    *scanner = Scanner_Get();
  ViSession        vi = scanner->digitizer->vi;
  ViChar        *chan = scanner->digitizer->config.acquisition_channels;
  TaskHandle  ao_task = scanner->daq_ao;
  TaskHandle clk_task = scanner->daq_clk;

  Frame_From_Bytes(frm, (void**)&buf, &desc);  
  _fill_frame_description(desc);
  change_token = desc->change_token;
  ref = *desc;

  DAQERR( DAQmxStartTask (ao_task));
  do
  {
    DAQERR( DAQmxStartTask (clk_task));
    DIGERR( niScope_InitiateAcquisition(vi));

    dt_out = toc(&outer_clock);
    debug("iter: %d\ttime: %5.3g [out] %5.3g [in]\tbacklog: %9.0f Bytes\r\n",i, dt_out, dt_in, sizeof(TPixel)*niscope_get_backlog(vi) );
    toc(&inner_clock);
    DIGERR( niScope_FetchBinary16 (vi,
                                   chan,
                                   SCANNER_VIDEO_TASK_FETCH_TIMEOUT,//10.0, //(-1=infinite) (0.0=immediate) 
                                   width,
                                   (ViInt16*) buf,
                                   wfm));

    // Push the acquired data down the output pipes
    Asynq_Push( qwfm,(void**) &wfm, 0 );
#ifdef SCANNER_DEBUG_FAIL_WHEN_FULL
    if(  !Asynq_Push_Try( qdata,(void**) &frm ))
#elif defined( SCANNER_DEBUG_SPIN_WHEN_FULL )
    if(  !Asynq_Push( qdata,(void**) &frm, FALSE ))
#else
    error("Choose a push behavior for digitizer by compileing with the appropriate define.\r\n");
#endif
    { warning("Digitizer output queue overflowed.\r\n\tAborting acquisition task.\r\n");
      goto Error;
    }
    Frame_From_Bytes(frm, (void**)&buf, &desc ); // The push swapped the frame buffer
    memcpy(desc,&ref,sizeof(Frame_Descriptor));  // ...so update buf and desc

    DAQERR( DAQmxWaitUntilTaskDone (clk_task,DAQmx_Val_WaitInfinitely));
    dt_in  = toc(&inner_clock);
              toc(&outer_clock);
    DAQERR( DAQmxStopTask (clk_task));
    ++i;
  } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
  status = 0;
  debug("Scanner - Video task completed normally.\r\n");
Error:
  free( frm );
  free( wfm );
  niscope_debug_print_status(vi);
  DAQERR( DAQmxStopTask (ao_task) );
  DIGERR( niScope_Abort(vi) );
  return status;
}

//----------------------------------------------------------------------------
DeviceTask*
Scanner_Create_Task_Video(void)
{ return DeviceTask_Alloc(_Scanner_Task_Video_Cfg,
                          _Scanner_Task_Video_Proc);
}

