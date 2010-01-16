#include "stdafx.h"
#include "device-task-digitizer-fetch-forever.h"
#include "util-niscope.h"
#include "util-nidaqmx.h"
#include "device-scanner.h"
#include "device.h"
#include "frame.h"
#include "frame-interface-digitizer.h"

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

void dig_cfg( ViInt32 minRecordLength )
{ Scanner_Config   *scn_cfg = &(Scanner_Get()->config);
  Digitizer_Config *dig_cfg = &(Scanner_Get()->digitizer->config);
  ViSession              vi = Scanner_Get()->digitizer->vi;

  Digitizer_Channel_Config line_trigger_cfg = dig_cfg->channela[ scn_cfg->line_trigger_src ];
  
  ViReal64 verticalRange         = line_trigger_cfg.range
  ViInt32  numRecords            = scn_cfg->scans;

  ViReal64   refPosition         = 0.0;
  ViReal64   verticalOffset      = 0.0;
  ViInt32    verticalCoupling    = line_trigger_cfg.coupling;
  ViReal64   probeAttenuation    = 1.0;
  ViBoolean  enforceRealtime     = NISCOPE_VAL_TRUE;

  struct niScope_wfmInfo *wfmInfoPtr = NULL;
  ViReal64 *waveformPtr = NULL;

  Guarded_Assert( dig_cfg->channels[ scn_cfg->line_trigger_src ].enabled );
  
  DIGCHK( niScope_init (resourceName, NISCOPE_VAL_FALSE, NISCOPE_VAL_FALSE, &vi));
  niscope_cfg_rtsi_default( vi );
  DIGCHK( niScope_ConfigureVertical(vi, 
                                    line_trigger_cfg.name,       // channelName
                                    verticalRange,
                                    verticalOffset,
                                    verticalCoupling,
                                    probeAttenuation,
                                    NISCOPE_VAL_TRUE));
  DIGCHK( niScope_ConfigureHorizontalTiming(vi, 
                                            minSampleRate,
                                            minRecordLength,
                                            refPosition,
                                            numRecords,
                                            enforceRealtime));
  
  // Analog trigger for bidirectional scans
  DIGCHK( niScope_ConfigureTriggerEdge (vi,
                                        line_trigger_cfg.name,   // channelName
                                        0.0,                     // triggerLevel
                                        NISCOPE_VAL_POSITIVE,    // triggerSlope
                                        verticalCoupling,        // triggerCoupling
                                        0.0,                     // triggerHoldoff
                                        0.0 ));                  // triggerDelay
  // Wait for start trigger (frame sync) on PFI1
  DIGCHK( niScope_SetAttributeViString( vi,
                                        "",
                                        NISCOPE_ATTR_ACQ_ARM_SOURCE,
                                        NISCOPE_VAL_PFI_1 ));
}

//
// DAQ
//

#define DAQCHK(expr) Guarded_DAQmx( (expr), #expr, error )
#define N 4096

void daq_cfg(void)
{ Scanner    *scanner = Get_Scanner();
  Scanner_Config *cfg = &scanner->config;
  TaskHandle  cur_task=0;
  float64     data[N];
  int32       written;
  
  ViInt32     N = cfg->samples;
  float64     frame_time = cfg->scans / cfg->resonant_frequency; // 512 records / (7920 records/sec)    
  float64     freq = N/frame_time; // 4096 samples / 64 ms = 63 kS/s

  // sawtooth
  { int i=N;
    float64 A = cfg->galvo_vpp;
    while(i--)
      data[i] = A*((double)i/((double)N)-0.5);    // linear ramp from -A/2 to A/2
  }

  // set up ao task
  cur_task = scanner->daq_ao;
  DAQCHK( DAQmxCreateAOVoltageChan   (cur_task,
                                      cfg->galvo_channel,   //eg: "/Dev1/ao0"
                                      ""                    //?
                                      cfg->galvo_v_lim_min, //Volts eg: -10.0
                                      cfg->galvo_v_lim_max, //Volts eg:  10.0
                                      DAQmx_Val_Volts,      //?
                                      NULL));               //?
  DAQCHK( DAQmxCfgSampClkTiming      (cur_task,
                                      cfg->galvo_ctr,       // "Ctr1InternalOutput",
                                      freq,
                                      DAQmx_Val_Rising,
                                      DAQmx_Val_ContSamps,  // use continuous output so that counter stays in control
                                      N));
  DAQCHK( DAQmxCfgAnlgEdgeStartTrig  (cur_task,"APFI0",DAQmx_Val_Rising, 0.0));
//HERE XXX
  DAQCHK( DAQmxWriteAnalogF64        (cur_task,
                                      N,
                                      0,                           // autostart?
                                      10.0,                        // timeout (s) - to write?
                                      DAQmx_Val_GroupByChannel,
                                      data,
                                      &written,
                                      NULL));
  Guarded_Assert( written == N );

  // set up counter for sample clock
  cur_task = scanner->daq_clk;;
  DAQCHK( DAQmxCreateCOPulseChanFreq       ( cur_task,
                                            "Dev1/ctr1", 
                                             "", 
                                             DAQmx_Val_Hz, 
                                             DAQmx_Val_Low,
                                             0.0,
                                             freq,
                                             0.5 ));
  DAQCHK( DAQmxCfgImplicitTiming           ( cur_task, DAQmx_Val_FiniteSamps, N ));
  DAQCHK( DAQmxCfgDigEdgeStartTrig         ( cur_task, "AnalogComparisonEvent", DAQmx_Val_Rising ));
  DAQCHK( DAQmxSetArmStartTrigType         ( cur_task, DAQmx_Val_DigEdge ));
  //DAQCHK( DAQmxSetDigEdgeArmStartTrigSrc   ( cur_task, "PFI0" ));
  DAQCHK( DAQmxSetDigEdgeArmStartTrigSrc   ( cur_task, "RTSI2" ));
  DAQCHK( DAQmxSetDigEdgeArmStartTrigEdge  ( cur_task, DAQmx_Val_Rising ));
  *clk_task = cur_task;
  
  return;
}

void daq_clear(TaskHandle *ao_task, TaskHandle *clk_task)
{
  if( *clk_task!=0 ) {
    DAQmxStopTask(*clk_task);
    DAQmxClearTask(*clk_task);
  }
  if( *ao_task!=0 ) {
    DAQmxStopTask(*ao_task);
    DAQmxClearTask(*ao_task);
  }
  *ao_task = *clk_task = 0;
}

//
// MAIN'S
//

void test1(void)
{ TaskHandle ao_task=0, clk_task=0;
  ViSession  vi;
  niScope_wfmInfo *wfminfo = 0;
  void *data = 0;
  char chan[] = "0";
  ViInt32 width = 0.95*minSampleRate/resfreq; // Samples per wave

  // App init
  Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();
  
  // Hardware init
  daq_cfg(&ao_task, &clk_task);
  dig_cfg(&vi, chan, width);
  
  wfminfo = dig_alloc_wfm_info(vi, chan);  
  data    = dig_alloc_data(vi, chan);
  
  // Main loop
  { int i=0;
    TicTocTimer outer_clock = tic(),
                inner_clock = tic();
    double dt_in=0.0, dt_out=0.0;
    DAQCHK( DAQmxStartTask               (ao_task));
    do
    { 
      DAQCHK( DAQmxStartTask             (clk_task));
      DIGCHK( niScope_InitiateAcquisition(vi));
      
      dt_out = toc(&outer_clock);               
      debug("iter: %d\ttime: %5.3g [out] %5.3g [in]\tbacklog: %9.0f Bytes\r\n",i, dt_out, dt_in, sizeof(TPixel)*dig_get_backlog(vi) );
      toc(&inner_clock);
      DIGCHK( niScope_FetchBinary16 (vi,
                                     chan,
                                     10.0, //(-1=infinite) (0.0=immediate)
                                     width,
                                     (ViInt16*) data,
                                     wfminfo));
      DAQCHK( DAQmxWaitUntilTaskDone     (clk_task,DAQmx_Val_WaitInfinitely));
      dt_in  = toc(&inner_clock);
               toc(&outer_clock);
      DAQCHK( DAQmxStopTask              (clk_task));      
    } while(++i);
    
    DAQCHK( DAQmxStopTask(ao_task) );
  }
  
  // clean up
  dig_free_data(data);
  dig_free_wfm_info(wfminfo);
  dig_close( vi );
  daq_clear( &ao_task, &clk_task);
}

