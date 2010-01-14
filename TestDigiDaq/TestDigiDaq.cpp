// TestDigiDaq.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "util-nidaqmx.h"
#include "util-niscope.h"

//
// DIGITIZIER
//

// requires vi is defined
#define DIGCHK( expr ) (niscope_chk( vi, expr, #expr, error   ))

typedef ViInt16 TPixel;

void dig_cfg(ViSession *pvi, ViChar *channelName, ViInt32 minRecordLength )
{
   ViSession vi = VI_NULL;
   
   ViChar   resourceName[]        = "Dev3";
   ViReal64 verticalRange         = 0.3;
   ViReal64 minSampleRate         = 60000000;;
   ViInt32  numRecords            = 512; //1024 lines
   

   ViReal64   refPosition         = 0.0;
   ViReal64   verticalOffset      = 0.0;
   ViInt32    verticalCoupling    = NISCOPE_VAL_DC;
   ViReal64   probeAttenuation    = 1.0;
   ViBoolean  enforceRealtime     = NISCOPE_VAL_TRUE;
   ViReal64   timeout             = -1; //Infinite

   // Waveforms
   struct niScope_wfmInfo *wfmInfoPtr = NULL;
   ViReal64 *waveformPtr = NULL;
   
   DIGCHK( niScope_init (resourceName, NISCOPE_VAL_FALSE, NISCOPE_VAL_FALSE, &vi));
//-=-=-=-=-      TODO: replace with util_niscope function
   DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_START_TRIGGER           , "", NISCOPE_VAL_RTSI_0 ));
   DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_REF_TRIGGER             , "", NISCOPE_VAL_RTSI_1 ));
   DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_READY_FOR_START_EVENT   , "", NISCOPE_VAL_RTSI_2 ));
   DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_READY_FOR_REF_EVENT     , "", NISCOPE_VAL_RTSI_3 ));
   DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_READY_FOR_ADVANCE_EVENT , "", NISCOPE_VAL_RTSI_4 ));
   DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_END_OF_ACQUISITION_EVENT, "", NISCOPE_VAL_RTSI_5 ));
   DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_END_OF_RECORD_EVENT     , "", NISCOPE_VAL_RTSI_6 ));
   DIGCHK( niScope_ExportSignal( vi, NISCOPE_VAL_REF_CLOCK               , "", NISCOPE_VAL_RTSI_7 ));
//-=-=-=-=-
   DIGCHK( niScope_ConfigureVertical(vi, 
                                     channelName,
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
   
   // Analog trigger for records (bidirectional scan)
   DIGCHK( niScope_ConfigureTriggerEdge (vi,
                                         "0",                     // channelName
                                         0.0,                     // triggerLevel
                                         NISCOPE_VAL_POSITIVE,    // triggerSlope
                                         NISCOPE_VAL_DC,          // triggerCoupling
                                         0.0,                     // triggerHoldoff
                                         0.0 ));                  // triggerDelay
   // Wait for start trigger (frame sync) on PFI1
   DIGCHK( niScope_SetAttributeViString( vi,
                                         "",
                                         NISCOPE_ATTR_ACQ_ARM_SOURCE,
                                         NISCOPE_VAL_PFI_1 ));
   *pvi = vi;
}

void dig_close(ViSession vi)
{ if(vi) niScope_close(vi);
}

niScope_wfmInfo* dig_alloc_wfm_info(ViSession vi, const char *chan)
{ ViInt32  numWaveform;
  
  DIGCHK( niScope_ActualNumWfms(vi, chan, &numWaveform));
  return (niScope_wfmInfo*) malloc( numWaveform * sizeof( niScope_wfmInfo ) );
}

void dig_free_wfm_info( niScope_wfmInfo* buf )
{ if(buf) free(buf);
}

void *dig_alloc_data(ViSession vi, const char *chan)
{ ViInt32  numWaveform, reclen;
  
  DIGCHK( niScope_ActualNumWfms(vi, chan, &numWaveform));
  DIGCHK( niScope_ActualRecordLength( vi, &reclen ));
  
  return malloc( numWaveform * reclen * sizeof(TPixel) );  
}

void dig_free_data(void *data)
{ if(data) free(data);
}

//
// DAQ
//

#define DAQCHK(expr) Guarded_DAQmx( (expr), #expr, error )
#define N 4000

void daq_cfg(TaskHandle *ao_task, TaskHandle *clk_task)
{ TaskHandle  cur_task=0;
	float64     data[N];	
	float64     freq = 10000; // Hz	- sample rate
	int32       written;

  { int i=N;
	  while(i--)
		  data[i] = 5.0*(double)i/((double)N);    // linear ramp from 0.0 to 5.0 V
  }

  // set up ao task
  cur_task = 0;
	DAQCHK( DAQmxCreateTask            ("galvo",&cur_task));
	DAQCHK( DAQmxCreateAOVoltageChan   (cur_task,"/Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts  ,NULL));
	DAQCHK( DAQmxCfgSampClkTiming      (cur_task,
	                                   "Ctr1InternalOutput",
	                                    freq,
	                                    DAQmx_Val_Rising,
	                                    DAQmx_Val_ContSamps,         // use continuous output so that counter stays in control
	                                    N));
	DAQCHK( DAQmxCfgAnlgEdgeStartTrig  (cur_task,"APFI0",DAQmx_Val_Rising, 0.0));
	DAQCHK( DAQmxWriteAnalogF64        (cur_task,
                                      N,
	                                    0,                           // autostart?
	                                    10.0,                        // timeout (s)
	                                    DAQmx_Val_GroupByChannel,
	                                    data,
	                                    &written,
	                                    NULL));
	*ao_task = cur_task;

	// set up counter for sample clock
	cur_task = 0;
	DAQCHK( DAQmxCreateTask                  ( "clock",&cur_task));
	DAQCHK( DAQmxCreateCOPulseChanFreq       ( cur_task, "Dev1/ctr1", "", DAQmx_Val_Hz, DAQmx_Val_Low, 0.0, freq, 0.5 ));
	DAQCHK( DAQmxCfgImplicitTiming           ( cur_task, DAQmx_Val_FiniteSamps, N ));
	DAQCHK( DAQmxCfgDigEdgeStartTrig         ( cur_task, "AnalogComparisonEvent", DAQmx_Val_Rising ));
	DAQCHK( DAQmxSetArmStartTrigType         ( cur_task, DAQmx_Val_DigEdge ));
	DAQCHK( DAQmxSetDigEdgeArmStartTrigSrc   ( cur_task, "PFI0" ));
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
  ViInt32 width = 7500; // Samples

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
	  TicTocTimer clock = tic();
	  DAQCHK( DAQmxStartTask             ( ao_task));
	  do
	  { 
	    DAQCHK( DAQmxStartTask             (clk_task));
	    DIGCHK( niScope_InitiateAcquisition(vi));
	            debug("iter: %d time: %g\r\n",i, toc(&clock) );
	            //debug(",%g\r\n",toc(&clock) );
	    DIGCHK( niScope_FetchBinary16 (vi,
	                                   chan,
	                                   0.0, //immediate
	                                   width,
	                                   (ViInt16*) data,
	                                   wfminfo));
	    DAQCHK( DAQmxWaitUntilTaskDone     (clk_task,DAQmx_Val_WaitInfinitely));
	            toc(&clock);
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

int _tmain(int argc, _TCHAR* argv[])
{ test1();
	return 0;
}

