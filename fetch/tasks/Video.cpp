/*
 * Video.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/*
 * Aside(s)
 * --------
 * Q: When should a helper function be included in the class vs. as a static
 *    local?
 * A: For code reuse via inheritance.
 *
 *    One might want to subclass a Task to reuse most of the config or run
 *    code.  For example, the volume acquisition task is likely to use mostly
 *    the same setup as this Video class. (The run loop is different, but
 *    the setup is very similar).
 */

#include "stdafx.h"
#include "Video.h"
#include "../devices/scanner2D.h"
#include "../util/util-nidaqmx.h"

#define SCANNER_VIDEO_TASK_FETCH_TIMEOUT  10.0 //10.0, //(-1=infinite) (0.0=immediate)
                                                // Setting this to infinite can sometimes make the application difficult to quit
#if 1
#define scanner_debug(...) debug(__VA_ARGS__)
#else
#define scanner_debug(...)
#endif

#define DIGWRN( expr )  (niscope_chk( vi, expr, #expr, warning ))
#define DIGERR( expr )  (niscope_chk( vi, expr, #expr, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( vi, expr, #expr, warning ), Error)
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
  { namespace scanner
    {


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
      template<> void Video<i8 >::_config_digitizer(device::Scanner2D *scanner);
      template<> void Video<i16>::_config_digitizer(device::Scanner2D *scanner);

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
        this->update(scanner);

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
        ((device::Shutter*)scanner)->Bind();

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
      { int32 written,
                    N = scanner->config.nsamples;
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

      template<typename T>
      Frame_With_Interleaved_Lines
      _describe_actual_frame(device::Scanner2D *d, ViInt32 *precordsize, ViInt32 *pnwfm)
      { ViSession                   vi = ((device::Digitizer*)d)->vi;
        ViInt32                   nwfm;
        ViInt32          record_length;
        void                     *meta = NULL;

        DIGERR( niScope_ActualNumWfms(vi,
                                      ((device::Digitizer*)d)->config.acquisition_channels,
                                      &nwfm ) );
        DIGERR( niScope_ActualRecordLength(vi, &record_length) );

        u32 scans = d->config.nscans;
        Frame_With_Interleaved_Lines format( (u16) record_length,              // width
                                                   scans,                      // height
                                             (u8) (nwfm/scans),                // number of channels
                                                   TypeID<T>() );              // pixel type
        Guarded_Assert( format.nchan  > 0 );
        Guarded_Assert( format.height > 0 );
        Guarded_Assert( format.width  > 0 );
        *precordsize = record_length;
        *pnwfm       = nwfm;
        return format;
      }
      template Frame_With_Interleaved_Lines _describe_actual_frame<i8 >(device::Scanner2D*,ViInt32*,ViInt32*);
      template Frame_With_Interleaved_Lines _describe_actual_frame<i16>(device::Scanner2D*,ViInt32*,ViInt32*);

      template<class TPixel>
      unsigned int
      Video<TPixel>::run(device::Scanner2D *d)
      { asynq *qdata = d->out->contents[0],
              *qwfm  = d->out->contents[1];
        Frame *frm   = NULL; //(Frame*) Asynq_Token_Buffer_Alloc(qdata);
        Frame_With_Interleaved_Lines ref;
        struct niScope_wfmInfo *wfm = NULL; //(niScope_wfmInfo*) Asynq_Token_Buffer_Alloc(qwfm);
        ViInt32 nwfm;
        ViInt32 width;// = d->_compute_record_size(),
        int      i = 0,
            status = 1; // status == 0 implies success, error otherwise
        size_t nbytes,
               nbytes_info;

        TicTocTimer outer_clock = tic(),
                    inner_clock = tic();
        double dt_in=0.0,
               dt_out=0.0;

        ViSession        vi = ((device::Digitizer*)d)->vi;
        ViChar        *chan = ((device::Digitizer*)d)->config.acquisition_channels;
        TaskHandle  ao_task = d->ao;
        TaskHandle clk_task = d->clk;

        ref = _describe_actual_frame<TPixel>(d,&width,&nwfm);
        nbytes = ref.size_bytes();
        nbytes_info = nwfm*sizeof(struct niScope_wfmInfo);
        //
        frm = (Frame*)                  Guarded_Malloc(nbytes     ,"task::Video - Allocate actual frame.");
        wfm = (struct niScope_wfmInfo*) Guarded_Malloc(nbytes_info,"task::Video - Allocate waveform info");
        //
        ref.format(frm);

        d->Open(); // Open shutter: FIXME: Ambiguous function name.
        DAQJMP( DAQmxStartTask (ao_task));
        do
        {
          DAQJMP( DAQmxStartTask (clk_task));
          DIGJMP( niScope_InitiateAcquisition(vi));

          dt_out = toc(&outer_clock);
          //debug("iter: %d\ttime: %5.3g [out] %5.3g [in]\tbacklog: %9.0f Bytes\r\n",i, dt_out, dt_in, sizeof(TPixel)*niscope_get_backlog(vi) );
          toc(&inner_clock);
          DIGJMP( Fetch<TPixel>( vi,
                                  chan,
                                  SCANNER_VIDEO_TASK_FETCH_TIMEOUT,//10.0, //(-1=infinite) (0.0=immediate) // seconds
                                  width,
                                  (TPixel*) frm->data,
                                  wfm));
          //debug("\tGain: %f\r\n",wfm[0].gain);

          // Push the acquired data down the output pipes
          Asynq_Push( qwfm,(void**) &wfm, nbytes_info, 0 );
      #ifdef SCANNER_DEBUG_FAIL_WHEN_FULL                     //"fail fast"
          if(  !Asynq_Push_Try( qdata,(void**) &frm,nbytes ))
      #elif defined( SCANNER_DEBUG_SPIN_WHEN_FULL )           //"fail proof" - overwrites when full
          if(  !Asynq_Push( qdata,(void**) &frm, nbytes, FALSE ))
      #else
          error("Choose a push behavior by compiling with the appropriate define.\r\n");
      #endif
          { warning("Scanner output frame queue overflowed.\r\n\tAborting acquisition task.\r\n");
            goto Error;
          }
          ref.format(frm);
          dt_in  = toc(&inner_clock);
                   toc(&outer_clock);
          DAQJMP( DAQmxWaitUntilTaskDone (clk_task,DAQmx_Val_WaitInfinitely)); // FIXME: Takes forever.

          DAQJMP(DAQmxStopTask(clk_task));
          ++i;
        } while ( !d->is_stopping() );
        status = 0;
        debug("Scanner - Video task completed normally.\r\n");
      Finalize:
        d->Close(); // Close the shutter. FIXME: Ambiguous function name.
        free( frm );
        free( wfm );
        niscope_debug_print_status(vi);
        DAQERR( DAQmxStopTask (ao_task) );              // FIXME: ??? These will deadlock on a panic.
        DAQERR( DAQmxStopTask (clk_task) );
        DIGERR( niScope_Abort(vi) );
        return status;
      Error:
        goto Finalize;
      }
      template unsigned int Video<i8 >::run(device::Scanner2D *d);
      template unsigned int Video<i16>::run(device::Scanner2D *d);
      
    } //end namespace scanner
  }
}
