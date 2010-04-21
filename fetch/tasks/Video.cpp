/*
 * Video.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "stdafx.h"
#include "Video.h"
#include "../devices/scanner2D.h"

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

namespace fetch
{ namespace task
  {
    //get_type_id
    template<class TPixel> static inline TPixel_ID get_type_id(void);
    template               static inline TPixel_ID get_type_id< i8>(void) {return id_i8;}
    template               static inline TPixel_ID get_type_id<i16>(void) {return id_i16;}

    //get_fetch_func
    typedef ViStatus (*t_fetch_func)(ViSession,ViChar,ViReal64,ViInt32,void*,struct niScope_wfmInfo);
    template<class TPixel> static inline t_fetch_func get_fetch_func(void);
    template               static inline t_fetch_func get_fetch_func< i8>(void) {return niScope_FetchBinary8;}
    template               static inline t_fetch_func get_fetch_func<i16>(void) {return niScope_FetchBinary16;}

    template<class TPixel>
    void
    Video<TPixel>::_config_digitizer(device::Scanner2D *scanner)
    { device::Scanner2D::Config *scn_cfg = &(scanner->config);
      device::Digitizer::Config *dig_cfg = &((device::Digitizer*)scanner)->config;
      ViSession                       vi = ((device::Digitizer*)scanner)->vi;

      device::Digitizer::Channel_Config* line_trigger_cfg;

      ViReal64   refPosition         = 0.0;
      ViReal64   verticalOffset      = 0.0;
      ViReal64   probeAttenuation    = 1.0;
      ViBoolean  enforceRealtime     = NISCOPE_VAL_TRUE;

      // Select the trigger channel
      line_trigger_cfg = dig_cfg->channels + scn_cfg->line_trigger_src;
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
        { if( ichan != scn_cfg->line_trigger_src )
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
                                                scanner->_compute_record_size(),
                                                refPosition,
                                                scn_cfg->nscans,
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
    template void Video<i8 >::_config_digitizer(device::Scanner2D *scanner);
    template void Video<i16>::_config_digitizer(device::Scanner2D *scanner);

    static void
    _setup_ao_chan(TaskHandle cur_task,
                   double     freq,
                   device::Scanner2D::Config        *cfg,
                   device::LinearScanMirror::Config *lsm_cfg,
                   device::Pockels::Config          *pock_cfg)
    {
      char aochan[POCKELS_MAX_CHAN_STRING + LINEAR_SCAN_MIRROR__MAX_CHAN_STRING + 1];

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

    template<class TPixel>
    void
    Video<TPixel>::_config_daq(device::Scanner2D *scanner)
    { device::Scanner2D::Config  *cfg = &scanner->config;
      TaskHandle             cur_task = 0;

      ViInt32   N          = cfg->nsamples;
      float64   frame_time = cfg->nscans / cfg->frequency_Hz;      //  512 records / (7920 records/sec)
      float64   freq       = N/frame_time;                         // 4096 samples / 64 ms = 63 kS/s

      int32        written;

      //
      // VERTICAL
      //

      // set up ao task - vertical
      cur_task = scanner->ao;

      // Setup AO channels
      DAQERR( DAQmxClearTask(scanner->ao) );                   // Once a DAQ task is started, it needs to be cleared before restarting
      DAQERR( DAQmxCreateTask( "scanner2d-ao", &scanner->ao)); //
      _setup_ao_chan(scanner->ao,
                     freq,
                     &scanner->config,
                     &((device::LinearScanMirror*)scanner)->config,
                     &((device::Pockels*)scanner)->config);
      cur_task = scanner->ao;

      scanner->_generate_ao_waveforms();
      DAQERR( DAQmxWriteAnalogF64(cur_task,
                                  N,
                                  0,                           // autostart?
                                  0.0,                         // timeout (s) - to write - 0 causes write to fail if blocked at all
                                  DAQmx_Val_GroupByChannel,
                                  scanner->ao_workspace->contents,
                                 &written,
                                  NULL));
      Guarded_Assert( written == N );

      // set up counter for sample clock
      cur_task = scanner->clk;
      DAQERR( DAQmxCreateCOPulseChanFreq       ( cur_task,
                                                 cfg->ctr,      // "Dev1/ctr1"
                                                 "sample-clock",
                                                 DAQmx_Val_Hz,
                                                 DAQmx_Val_Low,
                                                 0.0,
                                                 freq,
                                                 0.5 ));
      DAQERR( DAQmxCfgImplicitTiming           ( cur_task, DAQmx_Val_FiniteSamps, N ));
      DAQERR( DAQmxCfgDigEdgeStartTrig         ( cur_task, "AnalogComparisonEvent", DAQmx_Val_Rising ));
      DAQERR( DAQmxSetArmStartTrigType         ( cur_task, DAQmx_Val_DigEdge ));
      DAQERR( DAQmxSetDigEdgeArmStartTrigSrc   ( cur_task, cfg->armstart ));
      DAQERR( DAQmxSetDigEdgeArmStartTrigEdge  ( cur_task, DAQmx_Val_Rising ));

      // Set up the shutter control
      { u8 closed = SCANNER_DEFAULT_SHUTTER_CLOSED;
        int32 written = 0;
        cur_task = ((device::Shutter*)scanner)->daqtask;
        DAQERR( DAQmxCreateDOChan(      cur_task,
                                        SCANNER_DEFAULT_SHUTTER_CHANNEL,
                                        "shutter-command",
                                        DAQmx_Val_ChanPerLine ));
        DAQERR( DAQmxStartTask( cur_task ) );                       // go ahead and start it
        scanner->Close(); //Close the shutter.... FIXME: function name are not descriptive...
      }

      return;
    }
    template void Video<i8 >::_config_daq(device::Scanner2D *scanner);
    template void Video<i16>::_config_daq(device::Scanner2D *scanner);


    template<class TPixel>
    unsigned int
    Video<TPixel>::config(device::Scanner2D *d)
    { _config_daq(d);
      _config_digitizer(d);

      debug("Scanner2D configured for Video\r\n");
      return 1; //success
    }
    template unsigned int Video<i8 >::config(device::Scanner2D* d);
    template unsigned int Video<i16>::config(device::Scanner2D* d);


    template<class TPixel>
    unsigned int
    Video<TPixel>::update(device::Scanner2D *scanner)
    { i32 written;
      scanner->_generate_ao_waveforms();
      DAQERR( DAQmxWriteAnalogF64(scanner->ao,
                                  N,
                                  0,                           // autostart?
                                  0.0,                         // timeout (s) - to write - 0 causes write to fail if blocked at all
                                  DAQmx_Val_GroupByChannel,
                                  scanner->ao_workspace->contents,
                                 &written,
                                  NULL));
      Guarded_Assert( written == N );
      return 1;
    }

    template<class TPixel>
    unsigned int
    Video<TPixel>::run(device::Scanner2D *d)
    {
    }

  }
}
