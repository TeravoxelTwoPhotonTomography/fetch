/**
  \file
  Microscope task.  Acquire stacks for each marked tile in a plane.

  \author Nathan Clack <clackn@janelia.hhmi.org>

  \copyright
  Copyright 2010 Howard Hughes Medical Institute.
  All rights reserved.
  Use is subject to Janelia Farm Research Campus Software Copyright 1.1
  license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */


#include "common.h"
#include "TimeSeriesDockWidget.h"
#include "StackAcquisition.h"
#include "Video.h"
#include "frame.h"
#include "devices\digitizer.h"
#include "devices\Microscope.h"
#include "devices\tiling.h"

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

#define CHKJMP(expr) if(!(expr)) {warning("%s(%d)"ENDL"\tExpression indicated failure:"ENDL"\t%s"ENDL,__FILE__,__LINE__,#expr); goto Error;}




//
// SurfaceFind -  microscope task
//
namespace fetch
{ namespace task
  { namespace microscope {

      //Upcasting
      unsigned int SurfaceFind::config(IDevice *d) {return config(dynamic_cast<device::Microscope*>(d));}
      unsigned int SurfaceFind::run   (IDevice *d) {return run   (dynamic_cast<device::Microscope*>(d));}

      unsigned int SurfaceFind::config(device::Microscope *d)
      {
        static task::scanner::ScanStack<u16> grabstack;
        std::string filename;

        Guarded_Assert(d);

        //Assemble pipeline here
        IDevice *cur;
        cur = d->configPipeline();

        d->__scan_agent.disarm(10000/*timeout_ms*/);
        int isok=d->__scan_agent.arm(&grabstack,&d->scanner)==0;        

        return isok; //success
Error:
        return 0;
      }

      static int _handle_wait_for_result(DWORD result, const char *msg)
      {
          return_val_if( result == WAIT_OBJECT_0  , 0 );
          return_val_if( result == WAIT_OBJECT_0+1, 1 );
          Guarded_Assert_WinErr( result != WAIT_FAILED );
          if(result == WAIT_ABANDONED_0)   warning("%s(%d)"ENDL "\tSurfaceFind: Wait 0 abandoned"ENDL "\t%s"ENDL, __FILE__, __LINE__, msg);
          if(result == WAIT_ABANDONED_0+1) warning("%s(%d)"ENDL "\tSurfaceFind: Wait 1 abandoned"ENDL "\t%s"ENDL, __FILE__, __LINE__, msg);
          if(result == WAIT_TIMEOUT)       warning("%s(%d)"ENDL "\tSurfaceFind: Wait timeout"ENDL     "\t%s"ENDL, __FILE__, __LINE__, msg);

          Guarded_Assert_WinErr( result != WAIT_FAILED );

          return -1;
      }

      static int run_and_wait(Agent* master, Agent* agent, int (*callback)(int,void*), void *params)
      {   int eflag=0;
          eflag |= agent->run() != 1;
          { HANDLE hs[] = {
              agent->_thread,
              master->_notify_stop};
            DWORD res;
            int   t;            
            res = WaitForMultipleObjects(2,hs,FALSE,INFINITE); // wait for scan to complete (or cancel)
            t = _handle_wait_for_result(res,"SurfaceFind::run - Wait for scanner to finish.");
            switch(t)
            {
            case 0:                            // in this case, the scanner thread stopped.  Nothing left to do.
              eflag |= agent->last_run_result(); // check the run result
              if(callback)
                eflag |= callback(eflag,params); // respond
            case 1:                            // in this case, the stop event triggered and must be propagated.
              eflag |= agent->stop(SCANNER2D_DEFAULT_TIMEOUT) != 1;
              break;
            default:                           // in this case, there was a timeout or abandoned wait
              eflag |= 1;                      // failure
            }
          } // end waiting block
          return eflag;
      }
/*
NOTE:
  - This guy changes the stage position in a way that might not be super controlled.  Need to be sure all stage motions are limited.
  - This is set as it's own task for testing, but will probably be called as part of another task for production use.
    So state changes (to the pipeline,etc) need to be controlled.
TODO:
  1. Assemble pipeline
     Pipeline needs to have the classify worker at the end that keeps track of where the surface is.
     After a stack is run, the context for that worker will have the threshold or a "none found" indicator.


*/
      unsigned int SurfaceFind::run(device::Microscope *dc)
      {
        unsigned int eflag = 0; // success
        TS_OPEN("surface-find.f32");
        CHKJMP(dc->__scan_agent.is_runnable());

        while(eflag==0 && !dc->_agent->is_stopping())
        { TS_TIC;
          
          // Move stage
          { Vector3f curpos_mm = dc->stage()->getTarget(); // use current target z for tilepos z
            double range_um=dc->surface_find().max_z()-dc->surface_find().min_z();
            curpos_mm[2] -= 0.001f*dc->surface_find().backup_frac()*range_um;// convert um to mm
            dc->stage()->setPos(curpos_mm);
          }

          eflag |= dc->runPipeline();
          eflag |= run_and_wait(dc->__self_agent,dc->__scan_agent); // perform the scan
          eflag |= dc->stopPipeline();         // wait till everything stops
///////HERE
          // [ ] readout 
          // [ ] continue search?
          TS_TOC;
        } // end loop over tiles
        eflag |= dc->stopPipeline();           // wait till the  pipeline stops
        
        if(!eflag)
        {
          // [ ] respond?  
        }

        TS_CLOSE;
        return eflag;
Error:
        TS_CLOSE;
        return 1;
      }

}}}  // namespace fetch::task::microscope
