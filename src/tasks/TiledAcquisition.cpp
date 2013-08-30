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
#include "TiledAcquisition.h"
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

namespace fetch
{

  namespace task
  {

    //
    // TiledAcquisition -  microscope task
    //

    namespace microscope {

      //Upcasting
      unsigned int TiledAcquisition::config(IDevice *d) {return config(dynamic_cast<device::Microscope*>(d));}
      unsigned int TiledAcquisition::run   (IDevice *d) {return run   (dynamic_cast<device::Microscope*>(d));}

      unsigned int TiledAcquisition::config(device::Microscope *d)
      {
        static task::scanner::ScanStack<u16> grabstack;
        std::string filename;

        Guarded_Assert(d);

        //Assemble pipeline here
        IDevice *cur;
        cur = d->configPipeline();

        CHKJMP(d->file_series.ensurePathExists());
        d->file_series.inc();
        filename = d->stack_filename();
        IDevice::connect(&d->disk,0,cur,0);
        Guarded_Assert( d->disk.close()==0 );
        //Guarded_Assert( d->disk.open(filename,"w")==0);

        d->__scan_agent.disarm(10000/*timeout_ms*/);
        int isok=d->__scan_agent.arm(&grabstack,&d->scanner)==0;
        d->stage()->tiling()->resetCursor();          // this is here so that run/stop cycles will pick up where they left off

        return isok; //success
Error:
        return 0;
      }

      static int _handle_wait_for_result(DWORD result, const char *msg)
      {
          return_val_if( result == WAIT_OBJECT_0  , 0 );
          return_val_if( result == WAIT_OBJECT_0+1, 1 );
          Guarded_Assert_WinErr( result != WAIT_FAILED );
          if(result == WAIT_ABANDONED_0)   warning("%s(%d)"ENDL "\tTiledAcquisition: Wait 0 abandoned"ENDL "\t%s"ENDL, __FILE__, __LINE__, msg);
          if(result == WAIT_ABANDONED_0+1) warning("%s(%d)"ENDL "\tTiledAcquisition: Wait 1 abandoned"ENDL "\t%s"ENDL, __FILE__, __LINE__, msg);
          if(result == WAIT_TIMEOUT)       warning("%s(%d)"ENDL "\tTiledAcquisition: Wait timeout"ENDL     "\t%s"ENDL, __FILE__, __LINE__, msg);

          Guarded_Assert_WinErr( result != WAIT_FAILED );

          return -1;
      }

      unsigned int TiledAcquisition::run(device::Microscope *dc)
      {
        std::string filename;
        unsigned int eflag = 0; // success
        Vector3f tilepos;
        TS_OPEN("timer-tiles.f32");
        CHKJMP(dc->__scan_agent.is_runnable());

        device::StageTiling* tiling = dc->stage()->tiling();
        tiling->resetCursor();

        while(eflag==0 && !dc->_agent->is_stopping() && tiling->nextInPlanePosition(tilepos))
        { TS_TIC;
          debug("%s(%d)"ENDL "\t[Tiling Task] tilepos: %5.1f %5.1f %5.1f"ENDL,__FILE__,__LINE__,tilepos[0],tilepos[1],tilepos[2]);
          filename = dc->stack_filename();
          dc->file_series.ensurePathExists();
          dc->disk.set_nchan(dc->scanner.get2d()->digitizer()->nchan());
          eflag |= dc->disk.open(filename,"w");
          if(eflag)
          {
            warning("Couldn't open file: %s"ENDL, filename.c_str());
            return eflag;
          }

          // Move stage
          Vector3f curpos = dc->stage()->getTarget(); // use current target z for tilepos z
          debug("%s(%d)"ENDL "\t[Tiling Task] curpos: %5.1f %5.1f %5.1f"ENDL,__FILE__,__LINE__,curpos[0]*1000.0f,curpos[1]*1000.0f,curpos[2]*1000.0f);
          //tilepos[2] = curpos[2]*1000.0f;             // unit conversion here is a bit awkward
          dc->stage()->setPos(0.001f*tilepos);        // convert um to mm

          curpos = dc->stage()->getTarget(); // use current target z for tilepos z
          debug("%s(%d)"ENDL "\t[Tiling Task] curpos: %5.1f %5.1f %5.1f"ENDL,__FILE__,__LINE__,curpos[0]*1000.0f,curpos[1]*1000.0f,curpos[2]*1000.0f);

          eflag |= dc->runPipeline();
          eflag |= dc->__scan_agent.run() != 1;

          { // Wait for stack to finish
            HANDLE hs[] = {
              dc->__scan_agent._thread,
              dc->__self_agent._notify_stop};
            DWORD res;
            int   t;

            // wait for scan to complete (or cancel)
            res = WaitForMultipleObjects(2,hs,FALSE,INFINITE);
            t = _handle_wait_for_result(res,"TiledAcquisition::run - Wait for scanner to finish.");
            switch(t)
            {
            case 0:                            // in this case, the scanner thread stopped.  Nothing left to do.
              eflag |= dc->__scan_agent.last_run_result(); // check the run result
              eflag |= dc->__io_agent.last_run_result();
              if(eflag==0) // remove this if statement to mark tiles as "error" tiles.  In practice, it seems it's ok to go back and reimage, so the if statement stays
                tiling->markDone(eflag==0);      // only mark the tile done if the scanner task completed normally
            case 1:                            // in this case, the stop event triggered and must be propagated.
              eflag |= dc->__scan_agent.stop(SCANNER2D_DEFAULT_TIMEOUT) != 1;
              break;
            default:                           // in this case, there was a timeout or abandoned wait
              eflag |= 1;                      // failure
            }
          } // end waiting block

          // Output and Increment files
          dc->write_stack_metadata();          // write the metadata
          eflag |= dc->disk.close();
          dc->file_series.inc();               // increment regardless of completion status
          eflag |= dc->stopPipeline();         // wait till everything stops
          TS_TOC;
        } // end loop over tiles
        eflag |= dc->stopPipeline();           // wait till the  pipeline stops
        TS_CLOSE;
        return eflag;
Error:
        TS_CLOSE;
        return 1;
      }

    } // namespace microscope

  }   // namespace task
}     // namespace fetch
