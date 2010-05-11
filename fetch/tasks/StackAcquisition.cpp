/*
 * StackAcquisition.cpp
 *
 *  Created on: May 10, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
#include "stdafx.h"
#include "StackAcquisition.h"

#define DIGWRN( expr )  (niscope_chk( vi, expr, #expr, warning ))
#define DIGERR( expr )  (niscope_chk( vi, expr, #expr, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( vi, expr, #expr, warning ), Error)
#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )  goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  namespace task
  {

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
        { scanner->_generate_ao_waveforms();
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

          d->Shutter::Open(); // Open shutter: FIXME: Ambiguous function name.
          DAQJMP(DAQmxStartTask(ao_task));
          for(z_um=ummin;z_um<ummax && !d->is_stopping();z_um+=umstep)
          { d->_generate_ao_waveforms(z_um);
            d->_write_ao();
            DAQJMP(DAQmxStartTask(clk_task));
            DIGJMP(niScope_InitiateAcquisition(vi));

            dt_out = toc(&outer_clock);
            //debug("iter: %d\ttime: %5.3g [out] %5.3g [in]\tbacklog: %9.0f Bytes\r\n",i, dt_out, dt_in, sizeof(TPixel)*niscope_get_backlog(vi) );
            toc(&inner_clock);
#if 1
            DIGJMP(Fetch<TPixel> (vi, chan, SCANNER_STACKACQ_TASK_FETCH_TIMEOUT,//10.0, //(-1=infinite) (0.0=immediate) // seconds
                                  width,
                                  (TPixel*) frm->data,
                                  wfm));
#endif

            //debug("\tGain: %f\r\n",wfm[0].gain);

            // Push the acquired data down the output pipes
            debug("Task: StackAcquisition<%s>: pushing wfm\r\n", TypeStr<TPixel> ());
            Asynq_Push(qwfm, (void**) &wfm, nbytes_info, 0);
            debug("Task: StackAcquisition<%s>: pushing frame\r\n", TypeStr<TPixel> ());
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
            ++i;
          };

          status = 0;
          debug("Scanner - Stack Acquisition task completed normally.\r\n");
          Finalize: d->Shutter::Close(); // Close the shutter. FIXME: Ambiguous function name.
          free(frm);
          free(wfm);
          niscope_debug_print_status(vi);
          DAQERR(DAQmxStopTask(ao_task)); // FIXME: ??? These will deadlock on a panic.
          DAQERR(DAQmxStopTask(clk_task));
          DIGERR(niScope_Abort(vi));
          return status;
          Error: goto Finalize;
        }

    }
  }
}
