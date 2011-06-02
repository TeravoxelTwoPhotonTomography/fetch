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


#include "common.h"
#include "StackAcquisition.h"
#include "Video.h"
#include "frame.h"
#include "devices\digitizer.h"
#include "devices\Microscope.h"

#if 0
#define DBG(...) debug(__VA_ARGS__);
#else
#define DBG(...)
#endif

#define DIGWRN( expr )  (niscope_chk( vi, expr, #expr, __FILE__, __LINE__, warning ))
#define DIGERR( expr )  (niscope_chk( vi, expr, #expr, __FILE__, __LINE__, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( vi, expr, #expr, __FILE__, __LINE__, warning ), Error)
#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )  goto_if_fail( 0==DAQWRN(expr), Error)

#define CHKERR( expr )  {if(expr) {error("Expression indicated failure:\r\n\t%s\r\n",#expr);}} 0 //( (expr), #expr, error  ))
#define CHKJMP( expr )  goto_if((expr),Error)

#if 0
#define SCANNER_DEBUG_FAIL_WHEN_FULL
#else
#define SCANNER_DEBUG_WAIT_WHEN_FULL
#endif


#ifdef SCANNER_DEBUG_FAIL_WHEN_FULL
#define SCANNER_PUSH(...) Chan_Next_Try(__VA_ARGS__)
#elif defined(SCANNER_DEBUG_WAIT_WHEN_FULL)
#define SCANNER_PUSH(...) Chan_Next(__VA_ARGS__)
#else
#error("An overflow behavior for the scanner should be specified");
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
      unsigned int StackAcquisition::config(IDevice *d) {return config(dynamic_cast<device::Microscope*>(d));}
      unsigned int StackAcquisition::run   (IDevice *d) {return run   (dynamic_cast<device::Microscope*>(d));}

      unsigned int StackAcquisition::config(device::Microscope *d)
      { 
        static task::scanner::ScanStack<i16> grabstack;
        std::string filename;

        Guarded_Assert(d);

        //Assemble pipeline here
        IDevice *cur;
        cur = d->configPipeline();

        d->file_series.ensurePathExists();
        d->file_series.inc();
        filename = d->stack_filename();
        IDevice::connect(&d->disk,0,cur,0);
        Guarded_Assert( d->disk.close()==0 );
        //Guarded_Assert( d->disk.open(filename,"w")==0);

        d->__scan_agent.arm(&grabstack,&d->scanner);                      // why was this arm_nowait?

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

      unsigned int StackAcquisition::run(device::Microscope *dc)
      { 
        std::string filename;
        unsigned int eflag = 0; // success
        
        Guarded_Assert(dc->__scan_agent.is_runnable());
        //Guarded_Assert(dc->__io_agent.is_running());

        filename = dc->stack_filename(); 
        
        dc->file_series.ensurePathExists();
        eflag |= dc->disk.open(filename,"w");
        if(eflag)
          return eflag;

        eflag |= dc->runPipeline();
        eflag |= dc->__scan_agent.run() != 1;


        //Chan_Wait_For_Writer_Count(dc->__scan_agent._owner->_out->contents[0],1);

        { HANDLE hs[] = {dc->__scan_agent._thread,          
                         dc->__self_agent._notify_stop};
          DWORD res;
          int   t;

          // wait for scan to complete (or cancel)
          res = WaitForMultipleObjects(2,hs,FALSE,INFINITE);
          t = _handle_wait_for_result(res,"StackAcquisition::run - Wait for scanner to finish.");
          switch(t)
          { case 0:       // in this case, the scanner thread stopped.  Nothing left to do.
              eflag |= 0; // success
              //break; 
            case 1:       // in this case, the stop event triggered and must be propagated.
              eflag |= dc->__scan_agent.stop(SCANNER2D_DEFAULT_TIMEOUT) != 1;
              break;
            default:      // in this case, there was a timeout or abandoned wait
              eflag |= 1; //failure              
          }
          
          // Output metadata and Increment file
          eflag |= dc->disk.close();
          dc->write_stack_metadata();
          dc->file_series.inc();
          
          //dc->connect(&dc->disk,0,dc->pipelineEnd(),0);
          
        }
        eflag |= dc->stopPipeline(); // wait till the  pipeline stops
        return eflag;
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
      template<class TPixel> unsigned int ScanStack<TPixel>::config (IDevice *d) {return config(dynamic_cast<device::Scanner3D*>(d));}
      template<class TPixel> unsigned int ScanStack<TPixel>::update (IDevice *d) {return update(dynamic_cast<device::Scanner3D*>(d));}

      template<class TPixel> unsigned int ScanStack<TPixel>::run    (IDevice *d) 
      {
        device::Scanner3D *s = dynamic_cast<device::Scanner3D*>(d);
        device::Digitizer::Config digcfg = s->_scanner2d._digitizer.get_config();
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
          warning("ScanStack<>::run() - Got invalid kind() for Digitizer.get_config\r\n");          
        }
        return 0; //failure
      }

      template<class TPixel>
        unsigned int
        ScanStack<TPixel>::
        config(device::Scanner3D *d)
        {
          d->onConfigTask();
          debug("Scanner3D configured for StackAcquisition<%s>\r\n", TypeStr<TPixel> ());
          return 1; //success
        }

      template<class TPixel>
        unsigned int
        ScanStack<TPixel>::
        update(device::Scanner3D *scanner)
        { 
          scanner->generateAO();          
          return 1;
        }

      template<class TPixel>
        unsigned int
        ScanStack<TPixel>::run_niscope(device::Scanner3D *d)
        {
          Chan *qdata = Chan_Open(d->_out->contents[0],CHAN_WRITE), 
               *qwfm  = Chan_Open(d->_out->contents[1],CHAN_WRITE);
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

          device::NIScopeDigitizer *dig = d->_scanner2d._digitizer._niscope;
          device::NIScopeDigitizer::Config digcfg = dig->get_config();

          ViSession vi = dig->_vi;
          ViChar *chan = const_cast<ViChar*>(digcfg.chan_names().c_str());                   
          
          ref = _describe_actual_frame_niscope<TPixel>(
            dig,                                 // NISCope Digitizer
            d->_scanner2d.get_config().nscans(), // number of scans
            &width,                              // width in pixels (queried from board)
            &nwfm);                              // this should be the number of scans (queried from board)
          nbytes = ref.size_bytes();
          nbytes_info = nwfm * sizeof(struct niScope_wfmInfo);
          //
          Chan_Resize(qdata, nbytes);
          Chan_Resize(qwfm, nbytes_info);
          frm = (Frame*) Chan_Token_Buffer_Alloc(qdata);
          wfm = (struct niScope_wfmInfo*) Chan_Token_Buffer_Alloc(qwfm);
          nbytes = Chan_Buffer_Size_Bytes(qdata);
          nbytes_info = Chan_Buffer_Size_Bytes(qwfm);
          //
          ref.format(frm);

          d->_zpiezo.getScanRange(&ummin,&ummax,&umstep);
          
          d->generateAORampZ((float)ummin); //d->_generate_ao_waveforms__z_ramp_step(ummin);
          d->writeAO();
          d->_scanner2d._shutter.Open();
          CHKJMP(d->_scanner2d._daq.startAO());          
          for(z_um=ummin+umstep;z_um<ummax && !d->_agent->is_stopping();z_um+=umstep)
          { 
            CHKJMP(d->_scanner2d._daq.startCLK());            
            DIGJMP(niScope_InitiateAcquisition(vi));

            dt_out = toc(&outer_clock);            
            toc(&inner_clock);
#if 1
            DIGJMP(Fetch<TPixel> (vi, chan, SCANNER_STACKACQ_TASK_FETCH_TIMEOUT,//10.0, //(-1=infinite) (0.0=immediate) // seconds
                                  width,
                                  (TPixel*) frm->data,
                                  wfm));
#endif

            // Push the acquired data down the output pipes
            DBG("Task: StackAcquisition<%s>: pushing wfm\r\n", TypeStr<TPixel> ());
            Chan_Next_Try(qwfm,(void**)&wfm,nbytes_info);
            DBG("Task: StackAcquisition<%s>: pushing frame\r\n", TypeStr<TPixel> ());
            if(CHAN_FAILURE( SCANNER_PUSH(qdata,(void**)&frm,nbytes) ))
            { warning("(%s:%d) Scanner output frame queue overflowed."ENDL"\tAborting stack acquisition task."ENDL,__FILE__,__LINE__);
              goto Error;
            }
            ref.format(frm);
            dt_in = toc(&inner_clock);
            toc(&outer_clock);
            CHKJMP(d->_scanner2d._daq.waitForDone(SCANNER2D_DEFAULT_TIMEOUT));
            d->_scanner2d._daq.stopCLK();            
            debug("Generating AO for z = %f\r\n.",z_um);
            d->generateAORampZ((float)z_um);
            d->writeAO();
            ++i;
          };

          status = 0;
          DBG("Scanner - Stack Acquisition task completed normally.\r\n");
Finalize: 
          d->_scanner2d._shutter.Shut();          
          free(frm);
          free(wfm);
          Chan_Close(qdata);
          Chan_Close(qwfm);
          niscope_debug_print_status(vi);  
          CHKERR(d->_scanner2d._daq.stopAO());
          CHKERR(d->_scanner2d._daq.stopCLK());
          DIGERR(niScope_Abort(vi));
          return status;
Error: 
          warning("Error occurred during ScanStack<%s> task.\r\n",TypeStr<TPixel>());
          d->_scanner2d._daq.stopCLK();
          goto Finalize;
        }

        template<class TPixel>
        unsigned int fetch::task::scanner::ScanStack<TPixel>::run_simulated( device::Scanner3D *d )
        {
          Chan *qdata = Chan_Open(d->_out->contents[0],CHAN_WRITE);
          Frame *frm   = NULL;
          device::SimulatedDigitizer *dig = d->_scanner2d._digitizer._simulated;
          Frame_With_Interleaved_Planes ref(
            dig->get_config().width(),
            d->_scanner2d.get_config().nscans()*2,
            3,TypeID<TPixel>());
          size_t nbytes;        
          int status = 1; // status == 0 implies success, error otherwise
          f64 z_um,ummax,ummin,umstep;
                  
          nbytes = ref.size_bytes();        
          Chan_Resize(qdata, nbytes);
          frm = (Frame*)Chan_Token_Buffer_Alloc(qdata);
          ref.format(frm);

          debug("Simulated Stack!\r\n");
          HERE;
          d->_zpiezo.getScanRange(&ummin,&ummax,&umstep);
          for(z_um=ummin+umstep;z_um<ummax && !d->_agent->is_stopping();z_um+=umstep)
          //for(int d=0;d<100;++d)
          { size_t pitch[4];
            size_t n[3];
            frm->compute_pitches(pitch);
            frm->get_shape(n);

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

            if(CHAN_FAILURE( SCANNER_PUSH(qdata,(void**)&frm,nbytes) ))
            { warning("Scanner output frame queue overflowed.\r\n\tAborting acquisition task.\r\n");
              goto Error;
            }
            ref.format(frm);
            DBG("Task: ScanStack<%s>: pushing frame\r\n",TypeStr<TPixel>());
          }
          HERE;
Finalize:
          Chan_Close(qdata);
          free( frm );
          return status; // status == 0 implies success, error otherwise
Error:
          warning("Error occurred during ScanStack<%s> task.\r\n",TypeStr<TPixel>());
          goto Finalize;
        }

        template<class TPixel>
        unsigned int fetch::task::scanner::ScanStack<TPixel>::run_alazar( device::Scanner3D *d )
        { Chan *qdata = Chan_Open(d->_out->contents[0],CHAN_WRITE);
          Chan_Close(qdata);
          warning("Implement me!\r\n");
          return 1;
        }
    }
  }
}
