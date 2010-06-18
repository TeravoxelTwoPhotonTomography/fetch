/*
 * Scanner2D.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "stdafx.h"
#include "scanner2D.h"
#include "../frame.h"

#define DIGWRN( expr )  (niscope_chk( vi, expr, #expr, warning ))
#define DIGERR( expr )  (niscope_chk( vi, expr, #expr, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( vi, expr, #expr, warning ), Error)
#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  namespace device
  {
  
    Scanner2D::Config::Config()
    :
      frequency_Hz      (SCANNER2D_DEFAULT_RESONANT_FREQUENCY),                  // Resonant frequency (1/full period)
      nscans            (SCANNER2D_DEFAULT_SCANS),                               // Number of bidirectional scans (full periods) per frame
      line_duty_cycle   (SCANNER2D_DEFAULT_LINE_DUTY_CYCLE),                     // Acquire samples during this fraction of the full period
      line_trigger_src  (SCANNER2D_DEFAULT_LINE_TRIGGER_SRC),                    // Digitizer channel corresponding to resonant velocity input.
      nsamples          (SCANNER2D_DEFAULT_AO_SAMPLES)                           // DAQ AO: Samples per frame                         
    { Guarded_Assert( sizeof(trigger ) >= sizeof(SCANNER2D_DEFAULT_LINE_TRIGGER  ) );
      Guarded_Assert( sizeof(armstart) >= sizeof(SCANNER2D_DEFAULT_FRAME_ARMSTART) );
      Guarded_Assert( sizeof(clock   ) >= sizeof(SCANNER2D_DEFAULT_DAQ_CLOCK     ) );
      Guarded_Assert( sizeof(ctr     ) >= sizeof(SCANNER2D_DEFAULT_DAQ_CTR       ) );
      Guarded_Assert( sizeof(ctr_alt ) >= sizeof(SCANNER2D_DEFAULT_DAQ_CTR_ALT   ) );
      memcpy(trigger ,SCANNER2D_DEFAULT_LINE_TRIGGER  ,sizeof(trigger ));
      memcpy(armstart,SCANNER2D_DEFAULT_FRAME_ARMSTART,sizeof(armstart));
      memcpy(clock   ,SCANNER2D_DEFAULT_DAQ_CLOCK     ,sizeof(clock   ));
      memcpy(ctr     ,SCANNER2D_DEFAULT_DAQ_CTR       ,sizeof(ctr     ));
      memcpy(ctr_alt ,SCANNER2D_DEFAULT_DAQ_CTR_ALT   ,sizeof(ctr_alt ));
    }

    Scanner2D::Scanner2D() :
      config(), ao(NULL), clk(NULL), ao_workspace(NULL), notify_daq_done(INVALID_HANDLE_VALUE)
    {
      // - ao_workspace is used to generate the data used for analog output during
      //   a frame. Need 2*nsamples b.c. there are two ao channels (the linear
      //   scan mirror and the pockels cell)
      ao_workspace = vector_f64_alloc(config.nsamples * 2);
      Guarded_Assert_WinErr( notify_daq_done = CreateEvent(NULL,FALSE,FALSE,NULL));
    }

    Scanner2D::~Scanner2D()
    { Guarded_Assert_WinErr__NoPanic(CloseHandle(notify_daq_done));
      if (this->detach()>0)
        warning("Could not cleanly detach Scanner2D. (addr = 0x%p)\r\n",this);
      vector_f64_free(ao_workspace);
    }

    ViInt32
    Scanner2D::_compute_record_size(void)
    {
      double duty = this->config.line_duty_cycle,
             rate = ((Digitizer*) (this))->config.sample_rate,
             freq = this->config.frequency_Hz;
      return (ViInt32)(duty * rate / freq);
    }

    Frame_With_Interleaved_Lines
    Scanner2D::_describe_frame()
    {
      ViInt32 samples_per_scan = _compute_record_size();
      Frame_With_Interleaved_Lines
          format((u16)(samples_per_scan),
                 config.nscans,
                 (u8) (((Digitizer*) (this))->config.num_channels),
                 id_i16);
      Guarded_Assert(format.nchan > 0);
      Guarded_Assert(format.height > 0);
      Guarded_Assert(format.width > 0);
      return format;
    }

    unsigned int
    Scanner2D::detach(void)
    {
      unsigned int status = 0; // success 0, error 1
      if (!this->disarm(SCANNER2D_DEFAULT_TIMEOUT))
        warning("Could not cleanly disarm Scanner2D.\r\n");
      status |= this->Shutter::detach();
      status |= this->Digitizer::detach();
      this->lock();
      if (clk)
      {
        debug("Scanner2D: Attempting to detach DAQ CLK channel. handle: 0x%p\r\n",
              clk);
        DAQJMP( DAQmxStopTask( clk ));
        DAQJMP( DAQmxClearTask( clk ));
      }
      if (ao)
      {
        debug("Scanner2D: Attempting to detach DAQ AO  channel. handle: 0x%p\r\n",
              ao);
        DAQJMP( DAQmxStopTask(  ao ));
        DAQJMP( DAQmxClearTask( ao ));
      }      
    Error:
      this->unlock();
      ao = NULL;
      clk = NULL;
      return status;
    }

    unsigned int
    Scanner2D::attach(void)
    {
      unsigned int status = 0; // success 0, error 1
      return_val_if(status |= this->Shutter::attach(),status);   //((Shutter*  )this)->attach(), status);
      return_val_if(status |= this->Digitizer::attach(),status); //((Digitizer*)this)->attach(), status);
      this->lock();
      Guarded_Assert(ao == NULL);
      Guarded_Assert(clk == NULL);
      DAQJMP( status = DAQmxCreateTask( "scanner-ao", &ao ));
      DAQJMP( status = DAQmxCreateTask( "scanner-clk", &clk));
      this->set_available();    
    Error:
      this->unlock();
      return status;
    }
    
    void
    Scanner2D::_config_digitizer()
    { device::Digitizer::Config *dig_cfg = &this->Digitizer::config;
      ViSession                       vi =  this->Digitizer::vi;
      device::Digitizer::Channel_Config* line_trigger_cfg;

      ViReal64   refPosition         = 0.0;
      ViReal64   verticalOffset      = 0.0;
      ViReal64   probeAttenuation    = 1.0;
      ViBoolean  enforceRealtime     = NISCOPE_VAL_TRUE;

      // Select the trigger channel
      line_trigger_cfg = dig_cfg->channels + config.line_trigger_src;
      Guarded_Assert( line_trigger_cfg->enabled );

      // Configure vertical for line-trigger channel
      niscope_cfg_rtsi_default( vi );
      DIGERR( niScope_ConfigureVertical(vi,
                                        line_trigger_cfg->name,       // channelName
                                        line_trigger_cfg->range,
                                        0.0,
                                        line_trigger_cfg->coupling,
                                        probeAttenuation,
                                        NISCOPE_VAL_TRUE));          // enabled

      // Configure vertical of other channels
      { int ichan = dig_cfg->num_channels;
        while( ichan-- )
        { if( ichan != config.line_trigger_src )
          { device::Digitizer::Channel_Config *c = dig_cfg->channels + ichan;
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

      // Configure horizontal -
      DIGERR( niScope_ConfigureHorizontalTiming(vi,
                                                dig_cfg->sample_rate,
                                                this->_compute_record_size(),
                                                refPosition,
                                                config.nscans,
                                                enforceRealtime));

      // Analog trigger for bidirectional scans
      DIGERR( niScope_ConfigureTriggerEdge (vi,
                                            line_trigger_cfg->name,    // channelName
                                            0.0,                       // triggerLevel
                                            NISCOPE_VAL_POSITIVE,      // triggerSlope
                                            line_trigger_cfg->coupling,// triggerCoupling
                                            0.0,                       // triggerHoldoff
                                            0.0 ));                    // triggerDelay
      // Wait for start trigger (frame sync) on PFI1
      DIGERR( niScope_SetAttributeViString( vi,
                                            "",
                                            NISCOPE_ATTR_ACQ_ARM_SOURCE,
                                            NISCOPE_VAL_PFI_1 ));
      return;
    }

    void
    Scanner2D::_compute_galvo_waveform__constant_zero( Scanner2D::Config *cfg, float64 *data, double N )
    { memset(data,0, ((size_t)N*sizeof(float64)));
    }

    void
    Scanner2D::_compute_linear_scan_mirror_waveform__sawtooth( LinearScanMirror::Config *cfg, float64 *data, double N )
    { int i=(int)N;
      float64 A = cfg->vpp;
      while(i--)
        data[i] = A*((i/N)-0.5); // linear ramp from -A/2 to A/2
      data[(int)N-1] = data[0];  // at end of wave, head back to the starting position
    }

    void
    Scanner2D::_compute_pockels_vertical_blanking_waveform( Pockels::Config *cfg, float64 *data, double N )
    { int i=(int)N;
      float64 max = cfg->v_open,
              min = cfg->v_closed;
      while(i--)
        data[i] = max;           // step to max during y scan
      data[(int)N-1] = min;      // step to zero at end of scan
    }

    void
    Scanner2D::_generate_ao_waveforms(void)
    { int N = config.nsamples;
      f64 *m,*p;
      lock();
      vector_f64_request(ao_workspace, 2*N - 1 /*max index*/);
      m = ao_workspace->contents; // first the mirror data
      p = m + N;                  // then the pockels data
      _compute_linear_scan_mirror_waveform__sawtooth( &((LinearScanMirror*)this)->config, m, N);
      _compute_pockels_vertical_blanking_waveform(    &(         (Pockels*)this)->config, p, N);
      unlock();
    }



    static void
    _setup_ao_chan(TaskHandle cur_task,
                   double     freq,
                   device::Scanner2D::Config        *cfg,
                   device::LinearScanMirror::Config *lsm_cfg,
                   device::Pockels::Config          *pock_cfg)
    {
      char aochan[POCKELS_MAX_CHAN_STRING + LINEAR_SCAN_MIRROR__MAX_CHAN_STRING + 1];

      // concatenate the channel names
      memset(aochan, 0, sizeof(aochan));
      strcat(aochan, lsm_cfg->channel);
      strcat(aochan, ",");
      strcat(aochan, pock_cfg->ao_chan);

      DAQERR( DAQmxCreateAOVoltageChan(cur_task,
              aochan,                        //eg: "/Dev1/ao0,/Dev1/ao2"
              "vert-mirror-out,pockels-out", //name to assign to channel
              MIN( lsm_cfg->v_lim_min, pock_cfg->v_lim_min ), //Volts eg: -10.0
              MAX( lsm_cfg->v_lim_max, pock_cfg->v_lim_max ), //Volts eg:  10.0
              DAQmx_Val_Volts,               //Units
              NULL));                        //Custom scale (none)

      DAQERR( DAQmxCfgAnlgEdgeStartTrig(cur_task,
              cfg->trigger,
              DAQmx_Val_Rising,
              0.0));

      DAQERR( DAQmxCfgSampClkTiming(cur_task,
              cfg->clock,          // "Ctr1InternalOutput",
              freq,
              DAQmx_Val_Rising,
              DAQmx_Val_ContSamps, // use continuous output so that counter stays in control
              cfg->nsamples));
    }

    void
    Scanner2D::_config_daq()
    { TaskHandle             cur_task = 0;

      ViInt32   N          = config.nsamples;
      float64   frame_time = config.nscans / config.frequency_Hz;  //  512 records / (7920 records/sec)
      float64   freq       = N/frame_time;                         // 4096 samples / 64 ms = 63 kS/s

      // set up counter for sample clock
      // - A finite pulse sequence is generated by a pair of onboard counters.
      //   In testing, it appears that after the device is reset, initializing
      //   the counter task doesn't work quite right.  First, I have to start the
      //   task with the paired counter once.  Then, I can set things up normally.
      //   After initializing with the paired counter once, things work fine until
      //   the device (or computer) is reset.  My guess is this is a fault of the
      //   board or driver software.
      // - below, we just cycle the counters when config gets called.  This ensures
      //   everything configures correctly the first time, even after a device
      //   reset or cold start.

      // The "fake" initialization
      DAQERR( DAQmxClearTask(this->clk) );                  // Once a DAQ task is started, it needs to be cleared before restarting
      DAQERR( DAQmxCreateTask("scanner2d-clk",&this->clk)); //
      cur_task = this->clk;
      DAQERR( DAQmxCreateCOPulseChanFreq       ( cur_task,
                                                 config.ctr_alt,     // "Dev1/ctr0"
                                                 "sample-clock",
                                                 DAQmx_Val_Hz,
                                                 DAQmx_Val_Low,
                                                 0.0,
                                                 freq,
                                                 0.5 ));
      DAQERR( DAQmxStartTask(cur_task) );

      // The "real" initialization
      DAQERR( DAQmxClearTask(this->clk) );                  // Once a DAQ task is started, it needs to be cleared before restarting
      DAQERR( DAQmxCreateTask("scanner2d-clk",&this->clk)); //
      cur_task = this->clk;
      DAQERR( DAQmxCreateCOPulseChanFreq       ( cur_task,       // task
                                                 config.ctr,     // "Dev1/ctr1"
                                                 "sample-clock", // name
                                                 DAQmx_Val_Hz,   // units
                                                 DAQmx_Val_Low,  // idle state - resting state of the output terminal
                                                 0.0,            // delay before first pulse
                                                 freq,           // the frequency at which to generate pulse
                                                 0.5 ));         // duty cycle

      DAQERR( DAQmxCfgImplicitTiming           ( cur_task, DAQmx_Val_FiniteSamps, N ));
      DAQERR( DAQmxCfgDigEdgeStartTrig         ( cur_task, "AnalogComparisonEvent", DAQmx_Val_Rising ));
      DAQERR( DAQmxSetArmStartTrigType         ( cur_task, DAQmx_Val_DigEdge ));
      DAQERR( DAQmxSetDigEdgeArmStartTrigSrc   ( cur_task, config.armstart ));
      DAQERR( DAQmxSetDigEdgeArmStartTrigEdge  ( cur_task, DAQmx_Val_Rising ));

      //
      // VERTICAL
      //

      // set up ao task - vertical
      cur_task = this->ao;

      // Setup AO channels
      DAQERR( DAQmxClearTask(this->ao) );                   // Once a DAQ task is started, it needs to be cleared before restarting
      DAQERR( DAQmxCreateTask( "scanner2d-ao", &this->ao)); //
      DAQERR( DAQmxSetWriteRegenMode(this->ao,DAQmx_Val_DoNotAllowRegen));
      _setup_ao_chan(this->ao,
                     freq,
                     &this->config,
                     &this->LinearScanMirror::config,
                     &this->Pockels::config);
      this->_generate_ao_waveforms();
      this->_register_daq_event();

      // Set up the shutter control
      this->Shutter::Bind();

      return;
    }

    void
    Scanner2D::_write_ao(void)
    { int32 written,
                  N = this->config.nsamples;
      
      DAQERR( DAQmxWriteAnalogF64(this->ao,
                                  N,
                                  0,                           // autostart?
                                  0.0,                         // timeout (s) - to write - 0 causes write to fail if blocked at all
                                  DAQmx_Val_GroupByChannel,
                                  this->ao_workspace->contents,
                                 &written,
                                  NULL));
      Guarded_Assert( written == N );
      return;
    } 

    int32 CVICALLBACK _daq_event_callback(TaskHandle taskHandle, int32 type, uInt32 nsamples, void *callbackData)
    { Scanner2D *self = (Scanner2D*)(callbackData);
      Guarded_Assert_WinErr(SetEvent(self->notify_daq_done));
      return 0;
    }
    
    int32 CVICALLBACK _daq_event_done_callback(TaskHandle taskHandle, int32 sts, void *callbackData)
    { Scanner2D *self = (Scanner2D*)(callbackData);
      DAQERR(sts);
      Guarded_Assert_WinErr(SetEvent(self->notify_daq_done));
      return 0;
    }

    void 
    Scanner2D::_register_daq_event(void)
    { int32 N = this->config.nsamples; 
    
#if 0
      DAQERR( DAQmxRegisterEveryNSamplesEvent(
                  this->ao,                          // task handle
                  DAQmx_Val_Transferred_From_Buffer, // event type
                  N,                                 // number of samples
                  0,                                 // run the callback in a DAQmx thread
                  &_daq_event_callback,              // callback
                  (void*)this));                     // data passed to callback                                                
#else
      DAQERR( DAQmxRegisterDoneEvent (
                  this->clk,                         //task handle
                  0,                                 //run the callback in a DAQmx thread
                  &_daq_event_done_callback,         //callback
                  (void*)this));                     //data passed to callback 
#endif
    }
    
    unsigned int 
    Scanner2D::_wait_for_daq(DWORD timeout_ms) // returns 1 on success, 0 otherwise
    { HANDLE hs[] = {this->notify_daq_done,
                     this->notify_stop};
      DWORD  res;                     
      res = WaitForMultipleObjects(2,hs,FALSE,timeout_ms);
      if(res == WAIT_TIMEOUT)        error("Scanner2d::_wait_for_daq - Timed out waiting for DAQ to finish AO write.\r\n");
      else if(res==WAIT_ABANDONED_0)
      { warning("Scanner2D::_wait_for_daq - Abandoned wait on notify_daq_done.\r\n");
        return 0;
      }
      else if(res==WAIT_ABANDONED_0+1)
      { warning("Scanner2D::_wait_for_daq - Abandoned wait on notify_stop.\r\n");
        return 0;
      }
      return 1;      
    }
  }
}
