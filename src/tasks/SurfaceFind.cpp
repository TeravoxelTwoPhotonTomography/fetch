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
#include "devices\Microscope.h"
#include "tasks\SurfaceFind.h"

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
      { unsigned timeout_ms=10000;
        unsigned int eflag = 0;                                   // success
        cfg::device::Microscope       scope(dc->get_config());      // mutable copy of the config
        const cfg::device::Microscope original(dc->get_config()); // make a copy in order to restore settings later
        const cfg::tasks::SurfaceFind cfg=dc->get_config().surface_find();
        double range_um=cfg.max_um()-cfg.min_um();        
        Task* oldtask;
        static task::scanner::ScanStack<u16> scan;

        dc->transaction_lock();

        TS_OPEN("surface-find.f32");
        CHKJMP(dc->__scan_agent.is_runnable());

        // 1. Set up the stack acquisition
        { int nframes = scope.pipeline().frame_average_count();
          float step = cfg.dz_um()/(float)nframes;
          // z-scan range is inclusive
          scope.mutable_scanner3d()->mutable_zpiezo()->set_um_min(cfg.min_um());
          scope.mutable_scanner3d()->mutable_zpiezo()->set_um_max(cfg.max_um()); //ensure n frames are aquired
          scope.mutable_scanner3d()->mutable_zpiezo()->set_um_step(step);
        }
        dc->scanner.set_config(scope.scanner3d());

        // 2. [ ] setup pipeline

        while(eflag==0 && !dc->_agent->is_stopping() && !dc->surface_finder.any())
        { 

          TS_TIC;


          // 3. Move stage
          // [ ]: FIXME!  LIMIT TOTAL DISPLACEMENT
          { Vector3f curpos_mm = dc->stage()->getTarget(); // use current target z for tilepos z            
            curpos_mm[2] -= 0.001f*cfg.backup_frac()*range_um;// convert um to mm
            dc->stage()->setPos(curpos_mm);
          }

          // 4. Start the acquisition
          oldtask = dc->__scan_agent._task;
          CHKJMP(0==dc->__scan_agent.disarm(timeout_ms));
          CHKJMP(0==dc->__scan_agent.arm(&scan,&dc->scanner));

          eflag |= dc->runPipeline();
          eflag |= run_and_wait(&dc->__self_agent,&dc->__scan_agent,NULL,NULL); // perform the scan
          eflag |= dc->stopPipeline();         // wait till everything stops
          // [ ] readout
          if(dc->surface_finder.any())
          { float z_stack_um=dc->surface_finder.which()*cfg.dz_um()+cfg.min_um(); // stack displacement
            float z_stage_mm=dc->stage()->getTarget()[2]+1e-3*z_stack_um; //[ ] CHECK: double check sign
            // What to do with the answer?
            // - update stage position (lock to surface) and done.
            // - store somewhere.  Something else will get the data and update the stage position.
            //   That component would receive this data.  This is probably going to be the microscope.    
///////HERE
          }
          // [ ] continue search?
          TS_TOC;
        } // end - loop until surface found
        
        if(!eflag)
        {
          // [ ] respond?  
        }


        CHKJMP(dc->__scan_agent.stop());
        CHKJMP(0==dc->__scan_agent.disarm(timeout_ms));
Finalize:
        dc->stopPipeline(); //- redundant?
        dc->scanner.set_config(original.scanner3d());
        dc->__scan_agent._task=oldtask;
        dc->transaction_unlock();
        TS_CLOSE;
        return eflag;
Error:
        eflag=1;
        goto Finalize;
      }

}}}  // namespace fetch::task::microscope
