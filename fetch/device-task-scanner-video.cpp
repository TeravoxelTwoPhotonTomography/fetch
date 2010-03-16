
#include "stdafx.h"
#include "util-niscope.h"
#include "util-nidaqmx.h"
#include "device-digitizer.h"
#include "frame.h"
#include "device.h"
#include "device-scanner.h"
#include "device-task-scanner-video.h"

#define SCANNER_VIDEO_TASK_FETCH_TIMEOUT  10.0 //10.0, //(-1=infinite) (0.0=immediate)
                                                // Setting this to infinite can sometimes make the application difficult to quit
#if 1
#define scanner_debug(...) debug(__VA_ARGS__)
#else
#define scanner_debug(...)
#endif

#define DIGWRN( expr )  (niscope_chk( Scanner_Get()->digitizer->vi, expr, #expr, warning ))
#define DIGERR( expr )  (niscope_chk( Scanner_Get()->digitizer->vi, expr, #expr, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( Scanner_Get()->digitizer->vi, expr, #expr, warning ), Error)
#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )  goto_if_fail( 0==DAQWRN(expr), Error)

#if 1
#define SCANNER_DEBUG_FAIL_WHEN_FULL
#else
#define SCANNER_DEBUG_SPIN_WHEN_FULL
#endif

//
// Acquisition type setup
//
typedef ViInt16     TPixel;
#define TPixel_ID   id_i16
#define FETCHFUN    (niScope_FetchBinary16)

//typedef ViInt8      TPixel;
//#define TPixel_ID   id_i8
//#define FETCHFUN    (niScope_FetchBinary8)

//----------------------------------------------------------------------------
//
//  Video frame configuration
//

inline Frame_With_Interleaved_Lines
_format_frame( ViInt32 record_length, ViInt32 nwfm, f32 duty )
{ u32 scans = Scanner_Get()->config.scans;
  Frame_With_Interleaved_Lines format( (u16) record_length,              // width
                                             scans,                      // height
                                       (u8) (nwfm/scans),                // number of channels
                                             TPixel_ID );                // pixel type
  Guarded_Assert( format.nchan  > 0 );
  Guarded_Assert( format.height > 0 );
  Guarded_Assert( format.width  > 0 );
  return format;
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
                                    NISCOPE_VAL_TRUE));          // enabled
  // configure vertical of other channels
  { int ichan = dig_cfg->num_channels;
    while( ichan-- )
    { if( ichan != scn_cfg->line_trigger_src )
      { Digitizer_Channel_Config *c = dig_cfg->channels + ichan;
        DIGERR( niScope_ConfigureVertical(vi,
                                          c->name,       // channelName
                                          c->range,
                                          0.0,
                                          c->coupling,
                                          probeAttenuation,
                                          c->enabled));
      }
    }
  }
  
  
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

typedef void (*tfp_compute_gavlo_waveform)( Scanner_Config *cfg, float64 *data, double N );

void
_compute_galvo_waveform__constant_zero( Scanner_Config *cfg, float64 *data, double N )
{ memset(data,0, ((size_t)N*sizeof(float64)));
}

void
_compute_galvo_waveform__sawtooth( Scanner_Config *cfg, float64 *data, double N )
{ int i=(int)N;
  float64 A = cfg->galvo_vpp;
  while(i--)
    data[i] = A*((i/N)-0.5);    // linear ramp from -A/2 to A/2
  data[(int)N-1] = data[0];         // at end of wave, head back to the starting position
}

void
_compute_pockels_vertical_blanking_waveform( Scanner_Config *cfg, float64 *data, double N )
{ int i=(int)N;
  float64 max = cfg->pockels_v_open,
          min = cfg->pockels_v_closed;          
  while(i--)
    data[i] = max;      // step to max during y scan
  data[(int)N-1] = min; // step to zero at end of scan
}

// Configure DAQ for the frame program
// - slow mirror scan
// - frame sync
// - shutter
void _shutter_set( Scanner* scanner, u8 val )
{ int32 written = 0;
  DAQERR( DAQmxWriteDigitalLines( scanner->daq_shutter,
                                  1,                          // samples per channel,
                                  0,                          // autostart - set to false because this task will already be started by config function
                                  0,                          // timeout
                                  DAQmx_Val_GroupByChannel,   // data layout,
                                  &val,                       // buffer
                                  &written,                   // (out) samples written
                                  NULL ));                    // reserved
}

void _shutter_close( Scanner* scanner )
{ _shutter_set(scanner, SCANNER_DEFAULT_SHUTTER_CLOSED);
}

void _shutter_open( Scanner* scanner )
{ _shutter_set(scanner, SCANNER_DEFAULT_SHUTTER_OPEN);
  Sleep( SCANNER_DEFAULT_SHUTTER_DELAY /*ms*/ );
}

void DeviceTask_Scanner_Video_Write_Waveforms( Scanner *scanner )
{ float64        *data = NULL, *yscan, *pockels;
  Scanner_Config  *cfg = &scanner->config;
  int32        written;
  TaskHandle  cur_task = 0;
  
  ViInt32   N          = cfg->galvo_samples;
  float64   frame_time = cfg->scans / cfg->resonant_frequency; //  512 records / (7920 records/sec)
  float64   freq       = N/frame_time;                         // 4096 samples / 64 ms = 63 kS/s  
  
  data = (float64*) Guarded_Malloc( sizeof(float64) * N * 2, "scanner::task::write_waveforms" );
  yscan   = data;
  pockels = data + N;
  
  // fill in data for galvo waveform
  _compute_galvo_waveform__sawtooth( cfg, yscan, N );
  _compute_pockels_vertical_blanking_waveform(cfg, pockels, N );
    
  DAQERR( DAQmxClearTask(scanner->daq_ao) );
  DAQERR( DAQmxCreateTask( "frame-program", &scanner->daq_ao));
  cur_task = scanner->daq_ao;
  DAQERR( DAQmxCreateAOVoltageChan   (cur_task,
                                      cfg->galvo_channel,   //eg: "/Dev1/ao0"
                                      "galvo-mirror-out,pockels-out",   //name to assign to channel
                                      cfg->galvo_v_lim_min, //Volts eg: -10.0
                                      cfg->galvo_v_lim_max, //Volts eg:  10.0
                                      DAQmx_Val_Volts,      //Units
                                      NULL));               //Custom scale (none)

  DAQERR( DAQmxCfgAnlgEdgeStartTrig  (cur_task,
                                      cfg->galvo_trigger,
                                      DAQmx_Val_Rising,
                                      0.0));
  DAQERR( DAQmxCfgSampClkTiming      (cur_task,
                                      cfg->galvo_clock,     // "Ctr1InternalOutput",
                                      freq,
                                      DAQmx_Val_Rising,
                                      DAQmx_Val_ContSamps,  // use continuous output so that counter stays in control
                                      N));  
  DAQERR( DAQmxWriteAnalogF64        (scanner->daq_ao,
                                      N,
                                      0,                           // autostart?
                                      0.0,                         // timeout (s) - to write - 0 causes write to fail if blocked at all
                                      DAQmx_Val_GroupByChannel,
                                      data,
                                      &written,
                                      NULL));
  Guarded_Assert( written == N );
   
  free(data);
}
 
// FIXME: I broke the use of the <compute_galvo_waveform> argument.
//        Always uses sawtooth.
void _config_daq(tfp_compute_gavlo_waveform compute_gavlo_waveform) 
{ Scanner     *scanner = Scanner_Get();
  Scanner_Config  *cfg = &scanner->config;
  TaskHandle  cur_task = 0;
 
  

  ViInt32   N          = cfg->galvo_samples;
  float64   frame_time = cfg->scans / cfg->resonant_frequency; //  512 records / (7920 records/sec)
  float64   freq       = N/frame_time;                         // 4096 samples / 64 ms = 63 kS/s

  //
  // VERTICAL
  //

  // set up ao task - vertical
  cur_task = scanner->daq_ao;

  DeviceTask_Scanner_Video_Write_Waveforms( scanner );


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
  
  // Set up the shutter control
  { u8 closed = SCANNER_DEFAULT_SHUTTER_CLOSED;
    int32 written = 0;
    cur_task = scanner->daq_shutter;
    DAQERR( DAQmxCreateDOChan(      cur_task,
                                    SCANNER_DEFAULT_SHUTTER_CHANNEL,
                                    "shutter-command",
                                    DAQmx_Val_ChanPerLine ));
    DAQERR( DAQmxStartTask( cur_task ) );                       // go ahead and start it
    _shutter_close(scanner);
  }
  
  return;
}

Frame_With_Interleaved_Lines
_describe_frame()
{ ViSession                   vi = Scanner_Get()->digitizer->vi;
  ViInt32                   nwfm;
  ViInt32          record_length;
  void                     *meta = NULL;

  DIGERR( niScope_ActualNumWfms(vi, 
                                Scanner_Get()->digitizer->config.acquisition_channels,
                                &nwfm ) );
  DIGERR( niScope_ActualRecordLength(vi, &record_length) );

  return _format_frame( record_length, nwfm, Scanner_Get()->config.line_duty_cycle );
}

void
_config_pipes( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
// Allocate output pipes
{ ViInt32                   nwfm;
  ViSession                   vi = Scanner_Get()->digitizer->vi;
  char                     *chan = Scanner_Get()->digitizer->config.acquisition_channels;
  
  DIGERR( niScope_ActualNumWfms( vi,chan,&nwfm ) );    
  { size_t               nbuf[2] = { SCANNER_QUEUE_NUM_FRAMES,
                                     SCANNER_QUEUE_NUM_FRAMES },
                           sz[2] = { _describe_frame().size_bytes(),
                                     nwfm*sizeof(struct niScope_wfmInfo) };
    if( d->task->out == NULL )
      DeviceTask_Alloc_Outputs( d->task, 2, nbuf, sz );
  }
}

unsigned int
_Scanner_Task_Video_Cfg( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ _config_daq(_compute_galvo_waveform__sawtooth);
  _config_digitizer();
  _config_pipes(d,in,out);
  
  debug("Scanner configured for Video\r\n");
  return 1; //success
}

unsigned int
_Scanner_Task_Line_Scan_Cfg( Device *d, vector_PASYNQ *in, vector_PASYNQ *out )
{ _config_daq(_compute_galvo_waveform__constant_zero);
  _config_digitizer();
  _config_pipes(d,in,out);
  
  debug("Scanner configured for Line Scan\r\n");
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
  Frame_With_Interleaved_Lines ref;
  struct niScope_wfmInfo *wfm = (niScope_wfmInfo*) Asynq_Token_Buffer_Alloc(qwfm);
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

  ref = _describe_frame();
  ref.format(frm);

  _shutter_open(scanner);
  DAQJMP( DAQmxStartTask (ao_task));
  do
  {
    DAQJMP( DAQmxStartTask (clk_task));
    DIGJMP( niScope_InitiateAcquisition(vi));

    dt_out = toc(&outer_clock);
    //debug("iter: %d\ttime: %5.3g [out] %5.3g [in]\tbacklog: %9.0f Bytes\r\n",i, dt_out, dt_in, sizeof(TPixel)*niscope_get_backlog(vi) );
    toc(&inner_clock);
    DIGJMP( FETCHFUN (vi,
                      chan,
                      SCANNER_VIDEO_TASK_FETCH_TIMEOUT,//10.0, //(-1=infinite) (0.0=immediate) 
                      width,
                      (TPixel*) frm->data,
                      wfm));
    //debug("\tGain: %f\r\n",wfm[0].gain);

    // Push the acquired data down the output pipes
    Asynq_Push( qwfm,(void**) &wfm, 0 );
#ifdef SCANNER_DEBUG_FAIL_WHEN_FULL
    if(  !Asynq_Push_Try( qdata,(void**) &frm ))
#elif defined( SCANNER_DEBUG_SPIN_WHEN_FULL )
    if(  !Asynq_Push( qdata,(void**) &frm, FALSE ))
#else
    error("Choose a push behavior for digitizer by compileing with the appropriate define.\r\n");
#endif
    { warning("Scanner output frame queue overflowed.\r\n\tAborting acquisition task.\r\n");
      goto Error;
    }
    ref.format(frm);
    dt_in  = toc(&inner_clock);
             toc(&outer_clock);
    DAQJMP( DAQmxWaitUntilTaskDone (clk_task,DAQmx_Val_WaitInfinitely));
    
    DAQJMP(DAQmxStopTask(clk_task));
    ++i;
  } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
  status = 0;
  debug("Scanner - Video task completed normally.\r\n");
Finalize:
  _shutter_close(scanner);
  free( frm );
  free( wfm );
  niscope_debug_print_status(vi);
  DAQERR( DAQmxStopTask (ao_task) );              // FIXME: These will deadlock on error panic.
  DAQERR( DAQmxStopTask (clk_task) );
  DIGERR( niScope_Abort(vi) );
  return status;
Error:
  goto Finalize;
}

//----------------------------------------------------------------------------
DeviceTask*
Scanner_Create_Task_Video(void)
{ return DeviceTask_Alloc(_Scanner_Task_Video_Cfg,
                          _Scanner_Task_Video_Proc);
}

DeviceTask*
Scanner_Create_Task_Line_Scan(void)
{ return DeviceTask_Alloc(_Scanner_Task_Line_Scan_Cfg,
                          _Scanner_Task_Video_Proc);
}

