// TestDAQ.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "util-nidaqmx.h"

#define CHK(expr) Guarded_DAQmx( (expr), #expr, error )

#define N 4000

void test2(void)
{
	TaskHandle  cur_task=0, ao_task=0, clk_task=0;
	float64     data[N];	
	float64     freq = 10000; // Hz	- sample rate
	int32       written;

  { int i=N;
	  while(i--)
		  data[i] = 5.0*(double)i/((double)N);    // linear ramp from 0.0 to 5.0 V
  }

  Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();

  // set up ao task
  cur_task = 0;
	CHK( DAQmxCreateTask            ("galvo",&cur_task));
	CHK( DAQmxCreateAOVoltageChan   (cur_task,"/Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts  ,NULL));
	CHK( DAQmxCfgSampClkTiming      (cur_task,
	                                "Ctr0InternalOutput",
	                                 freq,
	                                 DAQmx_Val_Rising,
	                                 DAQmx_Val_ContSamps,         // use continuous output so that counter stays in control
	                                 N));
	CHK( DAQmxCfgAnlgEdgeStartTrig  (cur_task,"APFI0",DAQmx_Val_Rising, 0.0));
	CHK( DAQmxWriteAnalogF64        (cur_task,
	                                 N,
	                                 0,                           // autostart?
	                                 10.0,                        // timeout (s)
	                                 DAQmx_Val_GroupByChannel,
	                                 data,
	                                 &written,
	                                 NULL));
	ao_task = cur_task;

	// set up counter for sample clock
	cur_task = 0;
	CHK( DAQmxCreateTask                  ( "clock",&cur_task));                                        
	CHK( DAQmxCreateCOPulseChanFreq       ( cur_task, "Dev1/ctr0", "", DAQmx_Val_Hz, DAQmx_Val_Low, 0.0, freq, 0.5 ));
	CHK( DAQmxCfgImplicitTiming           ( cur_task, DAQmx_Val_FiniteSamps, N ));
	CHK( DAQmxCfgDigEdgeStartTrig         ( cur_task, "AnalogComparisonEvent", DAQmx_Val_Rising ));
	CHK( DAQmxSetArmStartTrigType         ( cur_task, DAQmx_Val_DigEdge ));
	CHK( DAQmxSetDigEdgeArmStartTrigSrc   ( cur_task, "PFI0" ));
	CHK( DAQmxSetDigEdgeArmStartTrigEdge  ( cur_task, DAQmx_Val_Rising ));
	clk_task = cur_task;
	
	{ int i;
	  TicTocTimer clock = tic();
	  CHK( DAQmxStartTask             (ao_task));
	  for(i=0;i<10; i++ )
	  { 
	    CHK( DAQmxStartTask             (clk_task));
	         //debug("iter: %d time: %g\r\n",i, toc(&clock) );
	         debug(",%g\r\n",toc(&clock) );
	    CHK( DAQmxWaitUntilTaskDone     (clk_task,100.0));
	         toc(&clock);
	    CHK( DAQmxStopTask              (clk_task));	    
	  }
	  DAQmxStopTask(ao_task);
	}

Error:

	if( clk_task!=0 ) {
		DAQmxStopTask(clk_task);
		DAQmxClearTask(clk_task);
	}
  if( ao_task!=0 ) {
		DAQmxStopTask(ao_task);
		DAQmxClearTask(ao_task);
	}
	printf("End of program, press Enter key to quit\n");
	getchar();
	return;
}


/* Timings   2010-01-11   Time for Start/Stop of AO task
a = array([8.17151e-005
,0.000236025
,0.000238579
,0.000252806
,0.000249523
,0.000233107
,0.000254995
,0.000231648
,0.000252806
,0.000253901])
*/

/* Timings   2010-01-12   (test2) Time for Start/Stop of doubley triggered ctr task
 *                                delay: 472 +- 6 us max: 483 us min: 459 us
a = array([0.000458918
,0.000482995
,0.000475334
,0.000474969
,0.000468768
,0.00047424
,0.000468768
,0.000476064
,0.000472416])
*/
int _tmain(int argc, _TCHAR* argv[])
{ test2();
}