#include "stdafx.h"

#include "fetch-forever.h"
#include "util-niscope.h"
#include "devices/digitizer.h"
#include "frame.h"


#define CheckWarn( expression )  (niscope_chk( Digitizer_Get()->vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( Digitizer_Get()->vi, expression, #expression, &error   ))
#define ViErrChk( expression )    goto_if( CheckWarn(expression), Error )

#if 0
#define digitizer_task_fetch_forever_debug(...) debug(__VA_ARGS__)
#else
#define digitizer_task_fetch_forever_debug(...)
#endif


namespace fetch
{ namespace task
  { namespace digitizer
    {
      
      template <class T>
        static inline Frame_With_Interleaved_Planes
        format_frame(ViInt32 record_length, ViInt32 nwfm )
      { Frame_With_Interleaved_Planes fmt( (u8) (record_length/512), // width
                                                512,                 // height
                                          (u8)  nwfm,                // # channels
                                                TypeID<T>() );       // pixel type
        return fmt;
      }

      template<>
        static inline Frame_With_Interleaved_Planes
        format_frame<i8>(ViInt32 record_length, ViInt32 nwfm );

      template<>
        static inline Frame_With_Interleaved_Planes
        format_frame<i16>(ViInt32 record_length, ViInt32 nwfm );

      template<class TPixel>
        unsigned int
        FetchForever::config(Digitizer *dig)
        { ViSession   vi = dig->vi;
          Digitizer::Config cfg = dig->config;
          ViInt32 i;
          int nchan = 0;
          
          // Vertical
          for(i=0; i<cfg.num_channels; i++)
          { Digitizer::Channel_Config ch = cfg.channels[i];
            CheckPanic(niScope_ConfigureVertical (vi, 
                                                  ch.name,     //channelName, 
                                                  ch.range,    //verticalRange, 
                                                  0.0,         //verticalOffset, 
                                                  ch.coupling, //verticalCoupling, 
                                                  1.0,         //probeAttenuation, 
                                                  ch.enabled));//enabled?
            nchan += ch.enabled;
          }
          // Horizontal
          CheckPanic (niScope_ConfigureHorizontalTiming (vi, 
                                                        cfg.sample_rate,       // sample rate (S/s)
                                                        cfg.record_length,     // record length (S)
                                                        cfg.reference_position,// reference position (% units???)
                                                        cfg.num_records,       // number of records to fetch per acquire
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

      template<> unsigned int FetchForever<i8>::config(Digitizer *dig);
      template<> unsigned int FetchForever<i16>::config(Digitizer *dig);

      //get_fetch_func
      // TODO: try template constant
      typedef ViStatus (*t_fetch_func)(ViSession,ViChar,ViReal64,ViInt32,void*,struct niScope_wfmInfo);
      template<class TPixel> static inline t_fetch_func get_fetch_func(void);
      template               static inline t_fetch_func get_fetch_func< i8>(void) {return niScope_FetchBinary8;}
      template               static inline t_fetch_func get_fetch_func<i16>(void) {return niScope_FetchBinary16;}

      template<class T> static inline
        ViStatus _fetch(ViSession vi, ViChar *chan, double t, ViInt32 nelem, T *data, niScope_wfmInfo *wfm)
      { return (*get_fetch_func<T>())(vi,chan,t,nelem,(ViInt8*)data,wfm);
      }

      template<> static inline ViStatus _fetch<i8 >(ViSession,ViChar*,double,ViInt32,i8*, struct niScope_wfmInfo*);
      template<> static inline ViStatus _fetch<i16>(ViSession,ViChar*,double,ViInt32,i16*,struct niScope_wfmInfo*);

      template<class TPixel>
        unsigned int
        FetchForever::run(Digitizer *dig)
        { ViSession   vi = dig->vi;
          ViChar   *chan = dig->config.acquisition_channels;
          asynq   *qdata = dig->out->contents[0],
                   *qwfm = dig->out->contents[1];
          ViInt32  nelem,
                    nwfm,
                     ttl = 0,ttl2=0,
               old_state;
          u32   nfetches = 0,
                nframes  = 0,
                every    = 32;  // must be power of two - used for reporting FPS
          TicTocTimer t, delay_clock;
          double delay, maxdelay = 0.0, accdelay = 0.0;
          u32    last_max_fetch = 0;  
          Frame                  *frm  = NULL; //(Frame*)            Asynq_Token_Buffer_Alloc(qdata);
          struct niScope_wfmInfo *wfm  = NULL; //(niScope_wfmInfo*)  Asynq_Token_Buffer_Alloc(qwfm);
          Frame_With_Interleaved_Planes ref;
          unsigned int ret = 1;
          size_t nbytes;

          // Compute image dimensions
          CheckPanic( niScope_ActualNumWfms(vi, chan, &nwfm ) );
          CheckPanic( niScope_ActualRecordLength(vi, &nelem) );
          ref = _format_frame<TPixel>( nelem, nwfm );
          nbytes = ref.size_bytes();
          //
          frm = Guarded_Malloc(ref.size_bytes(),"task::FetchForever - Allocate actual frame.");
          wfm = Guarded_Malloc(nwfm*sizeof(struct niScope_wfminfo),"task::FetchForver - Allocate waveform info");
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
              ViStatus sts = _fetch<TPixel>(vi, 
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
              if( sts != VI_SUCCESS )
              { niscope_chk(vi,sts, "niScope_Fetch*", warning);
                goto Error;
              }
              ++nfetches;     
              ttl += wfm->actualSamples;  // add the chunk size to the total samples count     
              Asynq_Push( qwfm,(void**) &wfm, 0 );    // Push (swap) the info from the last fetch
            } while(ttl!=nelem);
            
            // Handle the full buffer
            { //double dt;
#ifdef DIGITIZER_DEBUG_FAIL_WHEN_FULL
              if(  !Asynq_Push_Try( qdata,(void**) &frm, nbytes ))    //   Push buffer and reset total samples count
#elif defined( DIGITIZER_DEBUG_SPIN_WHEN_FULL )
              if(  !Asynq_Push( qdata,(void**) &frm, nbytes, FALSE )) //   Push buffer and reset total samples count
#else
                error("Choose a push behavior for digitizer by compileing with the appropriate define.\r\n");
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
          } while ( WAIT_OBJECT_0 != WaitForSingleObject(dig->notify_stop, 0) );
          digitizer_task_fetch_forever_debug("Digitizer Fetch_Forever - Running done -\r\n"
                "Task done: normal exit\r\n");
          ret = 0; //success
        Error:
          ViErrChk   (niScope_SetAttributeViInt32 (vi, NULL,
                                                  NISCOPE_ATTR_FETCH_RELATIVE_TO,
                                                  old_state ));
          free( frm );
          free( wfm );
          debug("Digitizer: nfetches: %u nframes: %u\r\n"
                "\tDelay - max: %g (on fetch %d) mean:%g\r\n"
                "\tTotal acquired samples %f MS\r\n",nfetches, nframes,maxdelay, last_max_fetch,accdelay/nfetches,ttl2/1024.0/1024.0);
          niscope_debug_print_status(vi);
          CheckPanic( niScope_Abort(vi) );
          return ret;
        }
      template<i8>  unsigned int FetchForever::run(Digitizer *dig);
      template<i16> unsigned int FetchForever::run(Digitizer *dig);

    } // namespace digitizer
  }   // namespace task
}     // namespace fetch
