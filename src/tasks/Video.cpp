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
#include "config.h"
#include "common.h"
#include "Video.h"
#include "devices/scanner2D.h"
#include "util/util-nidaqmx.h"
#include "agent.h"

#include "types.h"

#include "util/timestream.h"

#define SCANNER_VIDEO_TASK_FETCH_TIMEOUT  10.0  //10.0, //(-1=infinite) (0.0=immediate)
                                                // Setting this to infinite can sometimes make the application difficult to quit
#if 1
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define DIGWRN( expr )  (niscope_chk( vi, expr, #expr, __FILE__, __LINE__, warning ))
#define DIGERR( expr )  (niscope_chk( vi, expr, #expr, __FILE__, __LINE__, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( vi, expr, #expr, __FILE__, __LINE__, warning ), Error)
#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )  goto_if( DAQmxFailed(DAQWRN(expr)), Error)
#define CHKERR( expr )  {if(expr) {error("Expression indicated failure:\r\n\t%s\r\n",#expr);}} 0 //( (expr), #expr, error  ))
#define CHKJMP( expr )  goto_if((expr),Error)

#if 1 // PROFILING
#define TS_OPEN(name)   timestream_t ts__=timestream_open(name)
#define TS_TIC          timestream_tic(ts__)
#define TS_TOC          timestream_toc(ts__)
#define TS_CLOSE        timestream_close(ts__)
#else
#define TS_OPEN(name)
#define TS_TIC
#define TS_TOC
#define TS_CLOSE
#endif

#define LOG(...)     debug(__VA_ARGS__)
#if 0
#define ECHO(e)      LOG(e)
#else
#define ECHO(e)
#endif
#define REPORT(e)    LOG("%s(%d) - %s():"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,__FUNCTION__,e)
#define TRY(e)       do{ ECHO(#e); if(!(e)) {REPORT(#e); goto Error;}} while(0)

#if 1
#define SCANNER_DEBUG_FAIL_WHEN_FULL
#else
#define SCANNER_DEBUG_SPIN_WHEN_FULL
#endif

#ifdef SCANNER_DEBUG_FAIL_WHEN_FULL
#define SCANNER_PUSH(...) Chan_Next_Try(__VA_ARGS__)
#elif defined(SCANNER_DEBUG_SPIN_WHEN_FULL)
#define SCANNER_PUSH(...) Chan_Next(__VA_ARGS__)
#else
#error("An overflow behavior for the scanner should be specified");
#endif

namespace fetch
{ namespace task
  { namespace scanner
    {

      template class Video<i8 >;
      template class Video<i16>;
      template class Video<u16>;

      // config() and update() use Scanner2D's abstracted interface.
      // The run() functions rely on specific hardware API calls, so there's a different run function for each supported API.

      template<class TPixel> unsigned int Video<TPixel>::config (IDevice *d)//      {return config(dynamic_cast<device::Scanner2D*>(d));}
      {
        { device::Scanner2D *s = dynamic_cast<device::Scanner2D*>(d);
          if(s) return _config(s)==1;// else return 2;
        }
        { device::Scanner3D *s = dynamic_cast<device::Scanner3D*>(d);
          if(s) return _config(s)==1; else return 2;
        }
      }
      template<class TPixel> unsigned int Video<TPixel>::update (IDevice *d)      {
        { device::Scanner2D *s = dynamic_cast<device::Scanner2D*>(d);
          if(s) return _update(s);// else return 2;
        }
        { device::Scanner3D *s = dynamic_cast<device::Scanner3D*>(d);
          if(s) return _update(s); else return 2;
        }
      }  //      {return update(dynamic_cast<device::Scanner2D*>(d));}


      //template<class TPixel> unsigned int Video<TPixel>::run(device::Scanner3D* d)  {return run(&d->_scanner2d);}
      template<class TPixel> unsigned int Video<TPixel>::run(IDevice *d)
      {
        { device::Scanner2D *s = dynamic_cast<device::Scanner2D*>(d);
          if(s) return _run(s);// else return 2;
        }
        { device::Scanner3D *s = dynamic_cast<device::Scanner3D*>(d);
          if(s) return _run(s); else return 2;
        }
      }

      template<class TPixel> unsigned int Video<TPixel>::_run(device::IScanner* s)
      {
        //device::Scanner2D *s;
        //Guarded_Assert(s = dynamic_cast<device::Scanner2D*>(d));
        device::Digitizer::Config digcfg = s->get2d()->_digitizer.get_config();
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
          warning("Video<>::run() - Got invalid kind() for Digitizer.get_config\r\n");
        }
        return 0; //failure
      }


      template<class TPixel>
      unsigned int
      Video<TPixel>::_config(device::IScanner *d)
      {
        if(!d->onConfigTask())
          return 0; //failure.

        DBG("Scanner2D configured for Video<%s>\r\n",TypeStr<TPixel>());
        return 1; //success
      }


      template<class TPixel>
      unsigned int
      Video<TPixel>::_update(device::IScanner *scanner)
      {
        scanner->generateAO();
        return 1;
      }

#ifdef HAVE_NISCOPE
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
  #endif

      template<class TPixel>
      unsigned int
      Video<TPixel>::run_niscope(device::IScanner *d)
      {
#ifdef HAVE_NISCOPE
        Chan *qdata = Chan_Open(d->get2d()->_out->contents[0],CHAN_WRITE),
              *qwfm = Chan_Open(d->get2d()->_out->contents[1],CHAN_WRITE);
        Frame *frm   = NULL;
        Frame_With_Interleaved_Lines ref;
        struct niScope_wfmInfo *wfm = NULL;
        ViInt32 nwfm;
        ViInt32 width;
        int      i = 0,
            status = 1; // status == 0 implies success, error otherwise
        size_t nbytes,
               nbytes_info;
		TS_OPEN("timer-video_acq-niscope.f32");

        TicTocTimer outer_clock = tic(),
                    inner_clock = tic();
        double dt_in=0.0,
               dt_out=0.0;
        device::NIScopeDigitizer *dig = d->get2d()->_digitizer._niscope;
        device::NIScopeDigitizer::Config digcfg = dig->get_config();
        Guarded_Assert(dig!=NULL);
        ViSession        vi = dig->_vi;
        ViChar        *chan = const_cast<ViChar*>(digcfg.chan_names().c_str());

        ref = _describe_actual_frame_niscope<TPixel>(dig,d->get2d()->get_config().nscans(),&width,&nwfm);
        nbytes = ref.size_bytes();
        nbytes_info = nwfm*sizeof(struct niScope_wfmInfo);
        //
        Chan_Resize(qdata, nbytes);
        Chan_Resize(qwfm,  nbytes_info);
        frm = (Frame*)                  Chan_Token_Buffer_Alloc(qdata);
        wfm = (struct niScope_wfmInfo*) Chan_Token_Buffer_Alloc(qwfm);
        nbytes      = Chan_Buffer_Size_Bytes(qdata);
        nbytes_info = Chan_Buffer_Size_Bytes(qwfm);
        //
        ref.format(frm);

        //d->get2d()->_shutter.Open();
        d->writeAO();
        CHKJMP(d->get2d()->_daq.startAO());
        do
        {
          CHKJMP(d->get2d()->_daq.startCLK());
          DIGJMP( niScope_InitiateAcquisition(vi));

          dt_out = toc(&outer_clock);
          toc(&inner_clock);
		  TS_TIC;
#if 1
          DIGJMP( Fetch<TPixel>(vi,
                                chan,
                                SCANNER_VIDEO_TASK_FETCH_TIMEOUT,
                                width,
                                (TPixel*) frm->data,
                                wfm));
#endif
		  TS_TOC;

           // Push the acquired data down the output pipes
          DBG("Task: Video<%s>: pushing wfm\r\n",TypeStr<TPixel>());
          Chan_Next_Try( qwfm,(void**) &wfm, nbytes_info );
          DBG("Task: Video<%s>: pushing frame\r\n",TypeStr<TPixel>());
          if(CHAN_FAILURE( SCANNER_PUSH(qdata,(void**)&frm,nbytes) ))
          { warning("Scanner output frame queue overflowed.\r\n\tAborting acquisition task.\r\n");
            goto Error;
          }
          ref.format(frm);
          dt_in  = toc(&inner_clock);
                   toc(&outer_clock);

          CHKJMP(d->get2d()->_daq.waitForDone(SCANNER2D_DEFAULT_TIMEOUT));
          d->get2d()->_daq.stopCLK();
          d->writeAO();
          ++i;
        } while ( !d->get2d()->_agent->is_stopping() );

        status = 0;
        DBG("Scanner - Video task completed normally.\r\n");
Finalize:
		TS_CLOSE;
        //d->get2d()->_shutter.Shut();
        free( frm );
        free( wfm );
        Chan_Close(qdata);
        Chan_Close(qwfm);
        //niscope_debug_print_status(vi);

        CHKERR(d->get2d()->_daq.stopAO());
        CHKERR(d->get2d()->_daq.stopCLK());
        DIGERR( niScope_Abort(vi) );
        return status; // status == 0 implies success, error otherwise
Error:
        warning("Error occurred during Video<%s> task.\r\n",TypeStr<TPixel>());
        d->get2d()->_daq.stopAO();
        d->get2d()->_daq.stopCLK();
        goto Finalize;
#else // HAVE_NISCOPE
        return 1; //failure - no niscope
#endif
      }


      template<class TPixel>
      unsigned int fetch::task::scanner::Video<TPixel>::run_simulated( device::IScanner *d )
      { Chan *qdata = Chan_Open(d->get2d()->_out->contents[0],CHAN_WRITE);
        Frame *frm   = NULL;
        device::SimulatedDigitizer *dig = d->get2d()->_digitizer._simulated;
        Frame_With_Interleaved_Planes ref(
          dig->get_config().width(),
          d->get2d()->get_config().nscans()*2,
          3,
          TypeID<TPixel>());
        size_t nbytes;
        int status = 1; // status == 0 implies success, error otherwise
        size_t count = 0;
		TS_OPEN("timer-video_acq-simulated.f32");

        nbytes = ref.size_bytes();
        Chan_Resize(qdata, nbytes);
        frm = (Frame*)Chan_Token_Buffer_Alloc(qdata);
        ref.format(frm);

        DBG("Simulated Video!\r\n");
		
        while(!d->get2d()->_agent->is_stopping())
        { size_t pitch[4];
          size_t n[3];
          frm->compute_pitches(pitch);
          frm->get_shape(n);
		  TS_TIC;
#if 1
          //Fill frame w random colors.
          { TPixel *c,*e;
            const f32 low = TypeMin<TPixel>(),
                     high = TypeMax<TPixel>(),
                      ptp = high - low,
                      rmx = RAND_MAX;
            c=e=(TPixel*)frm->data;
            e+=pitch[0]/pitch[3];
            for(;c<e;++c)
              *c = (TPixel) ((ptp*rand()/(float)RAND_MAX) + low);
          }
#endif

#if 0
          // Walking px
          {
            const TPixel low = TypeMin<TPixel>(),
                        high = TypeMax<TPixel>();
            for(size_t  ichan=0;ichan<n[0];++ichan)
            {
              TPixel *p = (TPixel*)((u8*)frm->data + ichan*pitch[1]);
              memset(p,0,pitch[1]);
              p[(ichan*count)%pitch[1]]=low; //high;
            }
          }
#endif

          DBG("Task: Video<%s>: pushing frame\r\n",TypeStr<TPixel>());

          //frm->dump("simulated.%s",TypeStr<TPixel>());
		  TS_TOC;
          if(CHAN_FAILURE( SCANNER_PUSH(qdata,(void**)&frm,nbytes) ))
          { warning("Scanner output frame queue overflowed.\r\n\tAborting acquisition task.\r\n");
            goto Error;
          }
          ref.format(frm);
          ++count;
        }
Finalize:
		TS_CLOSE;
        Chan_Close(qdata);
        free( frm );
        return status; // status == 0 implies success, error otherwise
Error:
        warning("Error occurred during Video<%s> task.\r\n",TypeStr<TPixel>());
        goto Finalize;
      }

      //
      // --- ALAZAR ---
      //
      struct alazar_fetch_thread_ctx_t
      { device::IScanner *d;
        int running;       ///< used to indicate thread still running (thread to host)
        int ok;            ///< used to signal error state (bidirectional)
        Basic_Type_ID tid; ///< pixel type.  Used to allocate the frame.
        alazar_fetch_thread_ctx_t(device::IScanner *d,Basic_Type_ID tid)
          : d(d),tid(tid),running(1),ok(1) {}
      };

      DWORD alazar_fetch_video_thread(void *ctx_)
      { alazar_fetch_thread_ctx_t *ctx=(alazar_fetch_thread_ctx_t*)ctx_;
        device::Scanner2D *d=ctx->d->get2d();
        Chan *q = Chan_Open(d->_out->contents[0],CHAN_WRITE);
        device::AlazarDigitizer *dig=d->_digitizer._alazar;
        unsigned w,h,i;
        dig->get_image_size(&w,&h);
        size_t nbytes;
        Frame *frm   = NULL;
        Frame_With_Interleaved_Planes ref(w,h,dig->nchan(),ctx->tid);
        TS_OPEN("timer-video_acq.f32");
        nbytes = ref.size_bytes();
        Chan_Resize(q, nbytes);
        frm = (Frame*)Chan_Token_Buffer_Alloc(q);
        ref.format(frm);
        TRY(dig->start());
        //TS_TIC;
        while(!d->_agent->is_stopping() && ctx->ok)
        { TS_TIC;
          TRY(dig->fetch(frm));
          TS_TOC;
          TRY(CHAN_SUCCESS(SCANNER_PUSH(q,(void**)&frm,nbytes)));
          ref.format(frm);
        }
Finalize:
        TS_CLOSE;
        ctx->running=0;
        if(frm) free(frm);
        Chan_Close(q);
        return 0;
Error:
        ctx->ok=0;
        goto Finalize;
      }

      template<class TPixel>                     
      unsigned int fetch::task::scanner::Video<TPixel>::run_alazar( device::IScanner *d )
      { int ecode = 0; // ecode == 0 implies success, error otherwise
        HANDLE fetch_thread=0;
        alazar_fetch_thread_ctx_t ctx(d,TypeID<TPixel>());
        TS_OPEN("timer-video_ao.f32");
        d->generateAO();
        d->writeAO();
        TRY(!d->get2d()->_daq.startCLK());
        d->get2d()->_shutter.Open();
        TRY(!d->get2d()->_daq.startAO());
        Guarded_Assert_WinErr(fetch_thread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)alazar_fetch_video_thread,&ctx,0,NULL));
        TS_TIC;
        while(ctx.running)
        { TRY(!d->writeAO());
          TS_TOC;
        }
        if(ctx.ok)
          Guarded_Assert_WinErr(WAIT_OBJECT_0==WaitForSingleObject(fetch_thread,INFINITE));
        TRY(ctx.ok);
Finalize:
        TS_CLOSE;
        d->get2d()->_shutter.Shut();
        d->get2d()->_digitizer._alazar->stop();
        
        d->get2d()->_daq.waitForDone(1000/*ms*/); // will make sure the last frame finishes generating before it times out.

        d->get2d()->_daq.stopCLK();
        d->get2d()->_daq.stopAO();
        if(fetch_thread) CloseHandle(fetch_thread);
        return ecode; // ecode == 0 implies success, error otherwise
Error:
        warning("Error occurred during ScanStack<%s> task."ENDL,TypeStr<TPixel>());
        d->writeAO(); // try one more just in case
        ctx.ok=0; // signal fetch thread to stop early
        while(fetch_thread && ctx.running)
          Guarded_Assert_WinErr__NoPanic(WAIT_FAILED!=WaitForSingleObject(fetch_thread,100)); // need to make sure thread is stopped before exiting this function so ctx remains live
        ecode=1;
        goto Finalize;
      }

//end namespace fetch::task::scanner
    }
  }
}
