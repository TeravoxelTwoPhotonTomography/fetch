/*
 * StackAcquisition.cpp
 *
 *  Created on: May 10, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "stdafx.h"
#include "StackAcquisition.h"

#if 0
#define DBG(...) debug(__VA_ARGS__);
#else
#define DBG(...)
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
{

  namespace task
  {

    //
    // StackAcquisition -  microscope task
    //

    namespace microscope {

      //Upcasting
      unsigned int StackAcquisition::config(Agent *d) {return config(dynamic_cast<device::Microscope*>(d));}
      unsigned int StackAcquisition::run   (Agent *d) {return run   (dynamic_cast<device::Microscope*>(d));}

      unsigned int StackAcquisition::config(device::Microscope *agent)
      { static task::scanner::ScanStack<i8> grabstack;

        //Assemble pipeline here
        Agent *cur;
        cur = &agent->scanner;
        cur =  agent->pixel_averager.apply(cur);
        cur =  agent->cast_to_i16.apply(cur);
        cur =  agent->trash.apply(cur);

        agent->scanner.arm_nonblocking(&grabstack,INFINITE);

        return 1; //success
      }

      static int _handle_wait_for_result(DWORD result, const char *msg)
      {
          return_val_if( result == WAIT_OBJECT_0  , 0 );
          return_val_if( result == WAIT_OBJECT_0+1, 1 );
          Guarded_Assert_WinErr( result != WAIT_FAILED );
          if(result == WAIT_ABANDONED_0)
              warning("StackAcquisition: Wait 0 abandoned\r\n\t%s\r\n", msg);
          if(result == WAIT_ABANDONED_0+1)
              warning("StackAcquisition: Wait 1 abandoned\r\n\t%s\r\n", msg);

          if(result == WAIT_TIMEOUT)
              warning("StackAcquisition: Wait timeout\r\n\t%s\r\n", msg);

          Guarded_Assert_WinErr( result != WAIT_FAILED );

          return -1;
      }

      unsigned int StackAcquisition::run(device::Microscope *agent) {
        agent->scanner.run();

        { HANDLE hs[] = {agent->scanner.thread,          
                         agent->notify_stop};
          DWORD res;
          int   t;
          res = WaitForMultipleObjects(2,hs,FALSE,INFINITE);
          t = _handle_wait_for_result(res,"StackAcquisition::run - Wait for scanner to finish.");
          switch(t)
          { case 0:     // in this case, the scanner thread stopped.  Nothing left to do.
              return 0; // success
            case 1:     // in this case, the stop event triggered and must be propigated.
              return agent->scanner.stop(SCANNER2D_DEFAULT_TIMEOUT) != 1;              
            default:    // in this case, there was a timeout or abandoned wait
              return 1; //failure
          }
          
        }
        return 1;
      }

    }  // namespace microscope

    //
    // ScanStack - scanner task
    //

    namespace scanner
    {

      template class ScanStack<i8 >;
      template class ScanStack<i16>;

      // upcasts
      template<class TPixel> unsigned int ScanStack<TPixel>::config (Agent *d) {return config(dynamic_cast<device::Scanner3D*>(d));}
      template<class TPixel> unsigned int ScanStack<TPixel>::run    (Agent *d) {return run   (dynamic_cast<device::Scanner3D*>(d));}
      template<class TPixel> unsigned int ScanStack<TPixel>::update (Agent *d) {return update(dynamic_cast<device::Scanner3D*>(d));}

      template<class TPixel>
        unsigned int
        ScanStack<TPixel>::
        config(device::Scanner3D *d)
        {
          d->_config_daq();          
          d->_config_digitizer();

          debug("Scanner3D configured for StackAcquisition<%s>\r\n", TypeStr<TPixel> ());
          return 1; //success
        }

      template<class TPixel>
        unsigned int
        ScanStack<TPixel>::
        update(device::Scanner3D *scanner)
        { scanner->_generate_ao_waveforms(); // updates pockels, but sets z-piezo to 0 um          
          return 1;
        }

      template<typename T>
      Frame_With_Interleaved_Lines
      _describe_actual_frame(device::Scanner3D *d, ViInt32 *precordsize, ViInt32 *pnwfm)
      { ViSession                   vi = d->Digitizer::vi;
        ViInt32                   nwfm;
        ViInt32          record_length;
        void                     *meta = NULL;

        DIGERR( niScope_ActualNumWfms(vi,
                                      d->Digitizer::config.acquisition_channels,
                                      &nwfm ) );
        DIGERR( niScope_ActualRecordLength(vi, &record_length) );

        u32 scans = d->Scanner2D::config.nscans;
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


      template<class TPixel>
        unsigned int
        ScanStack<TPixel>::run(device::Scanner3D *d)
        {
          asynq *qdata = d->out->contents[0], *qwfm = d->out->contents[1];
          Frame *frm = NULL;
          Frame_With_Interleaved_Lines ref;
          struct niScope_wfmInfo *wfm = NULL;
          ViInt32 nwfm;
          ViInt32 width;
          int i = 0, status = 1; // status == 0 implies success, error otherwise
          size_t nbytes, nbytes_info;
          f64 z_um,ummax,ummin,umstep;

          TicTocTimer outer_clock = tic(), inner_clock = tic();
          double dt_in = 0.0, dt_out = 0.0;

          ViSession vi = d->Digitizer::vi;
          ViChar *chan = d->Digitizer::config.acquisition_channels;
          TaskHandle ao_task  = d->Scanner2D::ao;
          TaskHandle clk_task = d->Scanner2D::clk;

          ref = _describe_actual_frame<TPixel> (d, &width, &nwfm);
          nbytes = ref.size_bytes();
          nbytes_info = nwfm * sizeof(struct niScope_wfmInfo);
          //
          Asynq_Resize_Buffers(qdata, nbytes);
          Asynq_Resize_Buffers(qwfm, nbytes_info);
          frm = (Frame*) Asynq_Token_Buffer_Alloc(qdata);
          wfm = (struct niScope_wfmInfo*) Asynq_Token_Buffer_Alloc(qwfm);
          nbytes = qdata->q->buffer_size_bytes;
          nbytes_info = qwfm->q->buffer_size_bytes;
          //
          ref.format(frm);

          ummin  = d->ZPiezo::config.um_min;
          ummax  = d->ZPiezo::config.um_max;
          umstep = d->ZPiezo::config.um_step;

          
          d->_generate_ao_waveforms__z_ramp_step(ummin);
          d->_write_ao();
          d->Shutter::Open();
          DAQJMP(DAQmxStartTask(ao_task));
          for(z_um=ummin+umstep;z_um<ummax && !d->is_stopping();z_um+=umstep)
          { 
            //d->_write_ao();
            DAQJMP(DAQmxStartTask(clk_task));
            DIGJMP(niScope_InitiateAcquisition(vi));

            dt_out = toc(&outer_clock);
            //DBG("iter: %d\ttime: %5.3g [out] %5.3g [in]\tbacklog: %9.0f Bytes\r\n",i, dt_out, dt_in, sizeof(TPixel)*niscope_get_backlog(vi) );
            toc(&inner_clock);
#if 1
            DIGJMP(Fetch<TPixel> (vi, chan, SCANNER_STACKACQ_TASK_FETCH_TIMEOUT,//10.0, //(-1=infinite) (0.0=immediate) // seconds
                                  width,
                                  (TPixel*) frm->data,
                                  wfm));
#endif

            //DBG("\tGain: %f\r\n",wfm[0].gain);

            // Push the acquired data down the output pipes
            DBG("Task: StackAcquisition<%s>: pushing wfm\r\n", TypeStr<TPixel> ());
            Asynq_Push(qwfm, (void**) &wfm, nbytes_info, 0);
            DBG("Task: StackAcquisition<%s>: pushing frame\r\n", TypeStr<TPixel> ());
#ifdef SCANNER_DEBUG_FAIL_WHEN_FULL                     //"fail fast"
            if( !Asynq_Push_Try( qdata,(void**) &frm,nbytes ))
#elif defined( SCANNER_DEBUG_SPIN_WHEN_FULL )           //"fail proof" - overwrites when full
            if( !Asynq_Push( qdata,(void**) &frm, nbytes, FALSE ))
#else
            error("Choose a push behavior by compiling with the appropriate define.\r\n");
#endif
            {
              warning("Scanner output frame queue overflowed.\r\n\tAborting stack acquisition task.\r\n");
              goto Error;
            }
            ref.format(frm);
            dt_in = toc(&inner_clock);
            toc(&outer_clock);
            DAQJMP(DAQmxWaitUntilTaskDone(clk_task, DAQmx_Val_WaitInfinitely)); // FIXME: Takes forever.

            DAQJMP(DAQmxStopTask(clk_task));
            debug("Generating AO for z = %f\r\n.",z_um);
            d->_generate_ao_waveforms__z_ramp_step(z_um);
            d->_write_ao();
            ++i;
          };

          status = 0;
          DBG("Scanner - Stack Acquisition task completed normally.\r\n");
Finalize: 
          d->Shutter::Close(); // Close the shutter. FIXME: Ambiguous function name.
          free(frm);
          free(wfm);
          niscope_debug_print_status(vi);          
          DAQERR(DAQmxStopTask(ao_task)); // FIXME: ??? These will deadlock on a panic.
          DAQERR(DAQmxStopTask(clk_task));
          DIGERR(niScope_Abort(vi));
          return status;
Error: 
          goto Finalize;
        }

    }
  }
}
