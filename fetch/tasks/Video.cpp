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
#include "../agent.h"

#define SCANNER_VIDEO_TASK_FETCH_TIMEOUT  10.0  //10.0, //(-1=infinite) (0.0=immediate)
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

      template class Video<i8 >;
      template class Video<i16>;
      
      template<class TPixel> unsigned int Video<TPixel>::config (Agent *d) {return config(dynamic_cast<device::Scanner2D*>(d));}
      template<class TPixel> unsigned int Video<TPixel>::run    (Agent *d) {return run   (dynamic_cast<device::Scanner2D*>(d));}
      template<class TPixel> unsigned int Video<TPixel>::update (Agent *d) {return update(dynamic_cast<device::Scanner2D*>(d));}




      template<class TPixel>
      unsigned int
      Video<TPixel>::config(device::Scanner2D *d)
      { d->_config_daq();
        d->_config_digitizer();

        debug("Scanner2D configured for Video<%s>\r\n",TypeStr<TPixel>());
        return 1; //success
      }


      template<class TPixel>
      unsigned int
      Video<TPixel>::update(device::Scanner2D *scanner)
      { scanner->_generate_ao_waveforms();
        return 1;
      }

      template<typename T>
      Frame_With_Interleaved_Lines
      _describe_actual_frame(device::Scanner2D *d, ViInt32 *precordsize, ViInt32 *pnwfm);
      
      template Frame_With_Interleaved_Lines _describe_actual_frame<i8 >(device::Scanner2D*,ViInt32*,ViInt32*);
      template Frame_With_Interleaved_Lines _describe_actual_frame<i16>(device::Scanner2D*,ViInt32*,ViInt32*);
      
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
      

      template<class TPixel>
      unsigned int
      Video<TPixel>::run(device::Scanner2D *d)
      { asynq *qdata = d->out->contents[0],
              *qwfm  = d->out->contents[1];
        Frame *frm   = NULL;
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
        Asynq_Resize_Buffers(qdata, nbytes);
        Asynq_Resize_Buffers(qwfm,  nbytes_info);
        frm = (Frame*)                  Asynq_Token_Buffer_Alloc(qdata);
        wfm = (struct niScope_wfmInfo*) Asynq_Token_Buffer_Alloc(qwfm);
        nbytes      = qdata->q->buffer_size_bytes;
        nbytes_info = qwfm->q->buffer_size_bytes;
        //
        ref.format(frm);

        d->Shutter::Open(); // Open shutter: FIXME: Ambiguous function name.
        d->_write_ao();
        DAQJMP( DAQmxStartTask (ao_task));
        do
        { 
          DAQJMP( DAQmxStartTask (clk_task));
          DIGJMP( niScope_InitiateAcquisition(vi));

          dt_out = toc(&outer_clock);
          //debug("iter: %d\ttime: %5.3g [out] %5.3g [in]\tbacklog: %9.0f Bytes\r\n",i, dt_out, dt_in, sizeof(TPixel)*niscope_get_backlog(vi) );
          toc(&inner_clock);
#if 1                    
          DIGJMP( Fetch<TPixel>(vi,
                                chan,
                                SCANNER_VIDEO_TASK_FETCH_TIMEOUT,//10.0, //(-1=infinite) (0.0=immediate) // seconds
                                width,
                                (TPixel*) frm->data,
                                wfm));
#endif                                
          
          //debug("\tGain: %f\r\n",wfm[0].gain);

          // Push the acquired data down the output pipes
          debug("Task: Video<%s>: pushing wfm\r\n",TypeStr<TPixel>());
          Asynq_Push( qwfm,(void**) &wfm, nbytes_info, 0 );
          debug("Task: Video<%s>: pushing frame\r\n",TypeStr<TPixel>());
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
                   
          goto_if_fail(d->_wait_for_daq(SCANNER2D_DEFAULT_TIMEOUT),Error);
          //DAQJMP( DAQmxWaitUntilTaskDone (clk_task,DAQmx_Val_WaitInfinitely)); // FIXME: Takes forever.

          DAQJMP(DAQmxStopTask(clk_task));
          d->_write_ao();
          ++i;
        } while ( !d->is_stopping() );
        
        status = 0;
        debug("Scanner - Video task completed normally.\r\n");
      Finalize:
        d->Shutter::Close(); // Close the shutter. FIXME: Ambiguous function name.
        free( frm );
        free( wfm );
        niscope_debug_print_status(vi);
        DAQERR( DAQmxStopTask (ao_task) );              // FIXME: ??? These will deadlock on a panic.
        DAQERR( DAQmxStopTask (clk_task) );
        DIGERR( niScope_Abort(vi) );
        return status;
      Error:
        warning("Error occured during Video<%s> task.\r\n",TypeStr<TPixel>());
        goto Finalize;
      }
      
    } //end namespace scanner
  }
}
