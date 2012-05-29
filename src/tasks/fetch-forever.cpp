#include "common.h"
#include "fetch-forever.h"
#include "util/util-niscope.h"
#include "devices/digitizer.h"
#include "frame.h"


#define DIGWRN( expr )  (niscope_chk( vi, expr, #expr, __FILE__, __LINE__, warning ))
#define DIGERR( expr )  (niscope_chk( vi, expr, #expr, __FILE__, __LINE__, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( vi, expr, #expr, __FILE__, __LINE__, warning ), Error)

#define CheckWarn( expression )  (niscope_chk( vi, expression, #expression, __FILE__, __LINE__, &warning ))
#define CheckPanic( expression ) (niscope_chk( vi, expression, #expression, __FILE__, __LINE__, &error   ))
#define ViErrChk( expression )    goto_if( CheckWarn(expression), Error )

#if 0
#define digitizer_task_fetch_forever_debug(...) debug(__VA_ARGS__)
#else
#define digitizer_task_fetch_forever_debug(...)
#endif

#define DIGITIZER_DEBUG_FAIL_WHEN_FULL

namespace fetch
{ namespace task
  { namespace digitizer
    {
      template class FetchForever<i8>;
      template class FetchForever<i16>;
      
      template<class T> unsigned int FetchForever<T>::config (IDevice *d) {return config(dynamic_cast<device::NIScopeDigitizer*>(d));}
      template<class T> unsigned int FetchForever<T>::run    (IDevice *d) {return run   (dynamic_cast<device::NIScopeDigitizer*>(d));}
      
      template<typename T>
        static inline Frame_With_Interleaved_Planes
        _format_frame(ViInt32 record_length, ViInt32 nwfm )
      { Frame_With_Interleaved_Planes fmt( (u8) (record_length/512), // width
                                                512,                 // height
                                          (u8)  nwfm,                // # channels
                                                TypeID<T>() );       // pixel type
        return fmt;
      }

      template<typename TPixel>
        unsigned int
        FetchForever<TPixel>::config(device::NIScopeDigitizer *dig)
        { ViSession   vi = dig->_vi;
          device::NIScopeDigitizer::Config *cfg = dig->_config;
          ViInt32 i;
          int nchan = 0;
          
          // Vertical
          for(i=0; i<cfg->channel_size(); i++)
          { const device::NIScopeDigitizer::Config::Channel& ch = cfg->channel(i);          
            CheckPanic(niScope_ConfigureVertical (vi, 
                                                  ch.name().c_str(), //channelName, 
                                                  ch.range(),        //verticalRange, 
                                                  0.0,               //verticalOffset, 
                                                  ch.coupling(),     //verticalCoupling, 
                                                  1.0,               //probeAttenuation, 
                                                  ch.enabled()));    //enabled?
            nchan += ch.enabled();
          }
          // Horizontal
          CheckPanic (niScope_ConfigureHorizontalTiming (vi, 
                                                        cfg->sample_rate(),     // sample rate (S/s)
                                                        cfg->record_size(),     // record length (S)
                                                        cfg->reference(),       // reference position (% units???)
                                                        cfg->num_records(),     // number of records to fetch per acquire
                                                        NISCOPE_VAL_TRUE));    // enforce real time?
          // Configure software trigger, but never send the trigger.
          // This starts an infinite acquisition, until you call niScope_Abort
          // or niScope_close
          CheckPanic (niScope_ConfigureTriggerSoftware (vi, 
                                                        0.0,   // hold off (s)
                                                        0.0)); // delay    (s)

          digitizer_task_fetch_forever_debug("Digitizer configured for FetchForever\r\n");
          return 1;
        }


      template<class TPixel>
        unsigned int
        FetchForever<TPixel>::run(device::NIScopeDigitizer *dig)
        { ViSession   vi = dig->_vi;
          ViChar   *chan = const_cast<ViChar*>(dig->_config->chan_names().c_str());
          Chan    *qdata = Chan_Open(dig->_out->contents[0],CHAN_WRITE),
                   *qwfm = Chan_Open(dig->_out->contents[1],CHAN_WRITE);
          ViInt32  nelem,
                    nwfm,
                     ttl = 0,ttl2=0,
               old_state;
          u32   nfetches = 0,
                 nframes = 0,
                   every = 32;  // must be power of two - used for reporting FPS
          TicTocTimer t, delay_clock;
          double delay, maxdelay = 0.0, accdelay = 0.0;
          u32    last_max_fetch = 0;  
          Frame                  *frm  = NULL; //(Frame*)            Asynq_Token_Buffer_Alloc(qdata);
          struct niScope_wfmInfo *wfm  = NULL; //(niScope_wfmInfo*)  Asynq_Token_Buffer_Alloc(qwfm);
          Frame_With_Interleaved_Planes ref;
          unsigned int ret = 1;
          size_t nbytes, Bpp = sizeof(TPixel);

          // Compute image dimensions
          CheckPanic( niScope_ActualNumWfms(vi, chan, &nwfm ) );
          CheckPanic( niScope_ActualRecordLength(vi, &nelem) );
          ref = _format_frame<TPixel>( nelem, nwfm );
          nbytes = ref.size_bytes();
          //
          frm = (Frame*)                  Guarded_Malloc(ref.size_bytes()                   ,"task::FetchForever - Allocate actual frame.");
          wfm = (struct niScope_wfmInfo*) Guarded_Malloc(nwfm*sizeof(struct niScope_wfmInfo),"task::FetchForver - Allocate waveform info");
          //
          ref.format(frm);
          
          ViErrChk   (niScope_GetAttributeViInt32 (vi, NULL,   // TODO: reset to default when done
                                                  NISCOPE_ATTR_FETCH_RELATIVE_TO,         //?TODO: push/pop state for niscope?
                                                  &old_state ));    
          ViErrChk   (niScope_SetAttributeViInt32 (vi, NULL,   // TODO: reset to default when done
                                                  NISCOPE_ATTR_FETCH_RELATIVE_TO,         //?TODO: push/pop state for niscope?
                                                  NISCOPE_VAL_READ_POINTER ));

          // Loop until the stop event is triggered
          digitizer_task_fetch_forever_debug("Digitizer Fetch_Forever - Running -\r\n");
          t = tic();
          
          ViErrChk   (niScope_InitiateAcquisition (vi));
          delay_clock = tic();
          do 
          { ttl2 += ttl;
            ttl = 0;
            do
            { // Fetch the available data without waiting
              delay = toc( &delay_clock );
              delay = toc( &delay_clock );
              maxdelay = MAX(delay, maxdelay);
              ViStatus sts = Fetch<TPixel>(vi, 
                                            chan,          // (acquistion channels)
                                            0.0,           // Immediate
                                            nelem - ttl,   // Remaining space in buffer
                                            (TPixel*)frm->data + ttl,   // Where to put the data
                                            wfm);          // metadata for fetch
              if( delay > maxdelay )
              { maxdelay = delay;
                last_max_fetch = nfetches;
              }
              accdelay += delay;
              DIGJMP(sts);
              //if( sts != VI_SUCCESS )
              //{ niscope_chk(vi,sts, "niScope_Fetch*", warning);
              //  goto Error;
              //}
              ++nfetches;     
              ttl += wfm->actualSamples;  // add the chunk size to the total samples count     
              Chan_Next(qwfm,(void**)&wfm,nwfm*sizeof(struct niScope_wfmInfo));
            } while(ttl!=nelem);
            
            // Handle the full buffer
            { //double dt;
#ifdef DIGITIZER_DEBUG_FAIL_WHEN_FULL
              if(CHAN_FAILURE( Chan_Next_Try(qdata,(void**)&frm,nbytes) ))    //   Push buffer and reset total samples count
#elif defined( DIGITIZER_DEBUG_SPIN_WHEN_FULL )
              if(CHAN_FAILURE( Chan_Next(qdata,(void**)&frm,nbytes) )) //   Push buffer and reset total samples count
#else
#error("Choose a push behavior for digitizer by compiling with the appropriate define.\r\n");
#endif
              { warning("Digitizer output queue overflowed.\r\n\tAborting acquisition task.\r\n");
                goto Error;
              }
              ++nframes;
              { ViReal64 pts = 0;
                CheckPanic( niScope_GetAttributeViReal64( vi, NULL, NISCOPE_ATTR_BACKLOG, &pts ));
                digitizer_task_fetch_forever_debug("Digitizer Backlog: %4.1f MS\r\n",pts/1024.0/1024.0);
              }
              ref.format(frm); // format the new frame
              
              //dt = toc(&t);
              //if( !MOD_UNSIGNED_POW2(nframes+1,every) )
              //  debug("FPS: %3.1f Frame time: %5.4f MS/s: %3.1f  MB/s: %3.1f Q: %3d Digitizer\r\n",
              //        1.0/dt, dt, nelem/dt/1000000.0, nelem*sizeof(TPixel)*nwfm/1000000.0/dt,
              //        qdata->q->head - qdata->q->tail );
            }    
          } while ( !dig->_agent->is_stopping() );
          digitizer_task_fetch_forever_debug("Digitizer Fetch_Forever - Running done -\r\n"
                "Task done: normal exit\r\n");
          ret = 0; //success
        Error:
          ViErrChk   (niScope_SetAttributeViInt32 (vi, NULL,
                                                  NISCOPE_ATTR_FETCH_RELATIVE_TO,
                                                  old_state ));
          free( frm );
          free( wfm );
          Chan_Close(qdata);
          Chan_Close(qwfm);
          debug("Digitizer: nfetches: %u nframes: %u\r\n"
                "\tDelay - max: %g (on fetch %d) mean:%g\r\n"
                "\tTotal acquired samples %f MS\r\n",nfetches, nframes,maxdelay, last_max_fetch,accdelay/nfetches,ttl2/1024.0/1024.0);
          //niscope_debug_print_status(vi);
          CheckPanic( niScope_Abort(vi) );
          return ret;
        }

    } // namespace digitizer
  }   // namespace task
}     // namespace fetch
