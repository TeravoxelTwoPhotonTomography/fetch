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

#include "common.h"
#include "Video.h"
#include "devices/scanner2D.h"
#include "util/util-nidaqmx.h"
#include "agent.h"

#define SCANNER_VIDEO_TASK_FETCH_TIMEOUT  10.0  //10.0, //(-1=infinite) (0.0=immediate)
                                                // Setting this to infinite can sometimes make the application difficult to quit
#if 0
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define DIGWRN( expr )  (niscope_chk( vi, expr, #expr, warning ))
#define DIGERR( expr )  (niscope_chk( vi, expr, #expr, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( vi, expr, #expr, warning ), Error)
#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )  goto_if( DAQmxFailed(DAQWRN(expr)), Error)
#define CHKERR( expr )  {if(expr) {error("Expression indicated failure:\r\n\t%s\r\n",#expr);}} 0 //( (expr), #expr, error  ))
#define CHKJMP( expr )  goto_if((expr),Error)

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

      // config() and update() use Scanner2D's abstracted interface.
      // The run() functions rely on specific hardware API calls, so there's a different run function for each supported API.

      template<class TPixel> unsigned int Video<TPixel>::config (IDevice *d)      {return config(dynamic_cast<device::Scanner2D*>(d));}      
      template<class TPixel> unsigned int Video<TPixel>::update (IDevice *d)      {return update(dynamic_cast<device::Scanner2D*>(d));}
     
      template<class TPixel> unsigned int Video<TPixel>::run    (IDevice *d)
      { 
        device::Scanner2D *s = dynamic_cast<device::Scanner2D*>(d);
        device::Digitizer::Config digcfg = s->_digitizer.get_config();
        switch(digcfg.kind())
        {
        case cfg::device::Digitizer_DigitizerType_NIScope:
          return run_niscope(s);
          break;
        case cfg::device::Digitizer_DigitizerType_Alazar:
          return run_alazar(s);
          break;
        case cfg::device::Digitizer_DigitizerType_Simulated:
          return run_simulated(s);
          break;
        default:
          error("Video<>::run() - Got invalid kind() for Digitizer.get_config\r\n");          
        }
        return 0; //failure 
      }


      template<class TPixel>
      unsigned int
      Video<TPixel>::config(device::Scanner2D *d)
      { 
        d->onConfig();

        debug("Scanner2D configured for Video<%s>\r\n",TypeStr<TPixel>());
        return 1; //success
      }


      template<class TPixel>
      unsigned int
      Video<TPixel>::update(device::Scanner2D *scanner)
      { 
        scanner->generateAO();        
        return 1;
      }

      template<typename T>
      Frame_With_Interleaved_Lines
      _describe_actual_frame_niscope(device::NIScopeDigitizer *dig, ViInt32 nscans, ViInt32 *precordsize, ViInt32 *pnwfm);
      
      template Frame_With_Interleaved_Lines _describe_actual_frame_niscope<i8 >(device::NIScopeDigitizer*,ViInt32,ViInt32*,ViInt32*);
      template Frame_With_Interleaved_Lines _describe_actual_frame_niscope<i16>(device::NIScopeDigitizer*,ViInt32,ViInt32*,ViInt32*);
      
      template<typename T>
      Frame_With_Interleaved_Lines
      _describe_actual_frame_niscope(device::NIScopeDigitizer *dig, ViInt32 nscans, ViInt32 *precordsize, ViInt32 *pnwfm)
      { ViSession                   vi = dig->_vi; //d->device::NIScopeDigitizer::_vi;
        ViInt32                   nwfm;
        ViInt32          record_length;
        void                     *meta = NULL;

        DIGERR( niScope_ActualNumWfms(
          vi,
          dig->get_config().chan_names().c_str(),
          &nwfm ) );
        DIGERR( niScope_ActualRecordLength(vi, &record_length) );
#pragma warning(push)
#pragma warning(disable:4244)
        Frame_With_Interleaved_Lines format( (u16) record_length,              // width
                                                   nscans,                     // height
                                             (u8) (nwfm/nscans),               // number of channels
                                                   TypeID<T>() );              // pixel type
#pragma warning(pop)
        Guarded_Assert( format.nchan  > 0 );
        Guarded_Assert( format.height > 0 );
        Guarded_Assert( format.width  > 0 );
        *precordsize = record_length;
        *pnwfm       = nwfm;
        return format;
      }
      

      template<class TPixel>
      unsigned int
      Video<TPixel>::run_niscope(device::Scanner2D *d)
      { asynq *qdata = d->_out->contents[0],
              *qwfm  = d->_out->contents[1];
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
        device::NIScopeDigitizer *dig = d->_digitizer._niscope;
        device::NIScopeDigitizer::Config digcfg = dig->get_config();
        Guarded_Assert(dig!=NULL);
        ViSession        vi = dig->_vi; //d->device::NIScopeDigitizer::_vi;
        ViChar        *chan = const_cast<ViChar*>(digcfg.chan_names().c_str());//d->device::NIScopeDigitizer::config->chan_names().c_str());
        //TaskHandle  ao_task = d->ao;
        //TaskHandle clk_task = d->clk;

        ref = _describe_actual_frame_niscope<TPixel>(dig,d->get_config().nscans(),&width,&nwfm);
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

        d->_shutter.Open();
        d->writeAO();
        CHKJMP(d->_daq.startAO());
        do
        { 
          CHKJMP(d->_daq.startCLK());
          DIGJMP( niScope_InitiateAcquisition(vi));

          dt_out = toc(&outer_clock);
          toc(&inner_clock);
#if 1                    
          DIGJMP( Fetch<TPixel>(vi,
                                chan,
                                SCANNER_VIDEO_TASK_FETCH_TIMEOUT,
                                width,
                                (TPixel*) frm->data,
                                wfm));
#endif                                

           // Push the acquired data down the output pipes
          DBG("Task: Video<%s>: pushing wfm\r\n",TypeStr<TPixel>());
          Asynq_Push( qwfm,(void**) &wfm, nbytes_info, 0 );
          DBG("Task: Video<%s>: pushing frame\r\n",TypeStr<TPixel>());
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
                   
          CHKJMP(d->_daq.waitForDone(SCANNER2D_DEFAULT_TIMEOUT));
          d->_daq.stopCLK();
          d->writeAO();
          ++i;
        } while ( !d->_agent->is_stopping() );
        
        status = 0;
        DBG("Scanner - Video task completed normally.\r\n");
Finalize:
        d->_shutter.Shut();
        free( frm );
        free( wfm );
        niscope_debug_print_status(vi);

        CHKERR(d->_daq.stopAO());
        CHKERR(d->_daq.stopCLK());
        DIGERR( niScope_Abort(vi) );
        return status; // status == 0 implies success, error otherwise
Error:
        warning("Error occurred during Video<%s> task.\r\n",TypeStr<TPixel>());
        goto Finalize;
      }


      template<class TPixel>
      unsigned int fetch::task::scanner::Video<TPixel>::run_simulated( device::Scanner2D *d )
      {
        error("Implement me!\r\n");
        return 1;
      }

      template<class TPixel>
      unsigned int fetch::task::scanner::Video<TPixel>::run_alazar( device::Scanner2D *d )
      {
        error("Implement me!\r\n");
        return 1;
      }

//end namespace fetch::task::scanner
    } 
  }
}
