/*
 * TiledAcquisition.cpp
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
#include "TiledAcquisition.h"
#include "StackAcquisition.h"
#include "Video.h"
#include "frame.h"
#include "devices\digitizer.h"
#include "devices\Microscope.h"

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

        d->stage()->tiling().resetCursor();

        return 1; //success
      }

      static int _handle_wait_for_result(DWORD result, const char *msg)
      {
          return_val_if( result == WAIT_OBJECT_0  , 0 );
          return_val_if( result == WAIT_OBJECT_0+1, 1 );
          Guarded_Assert_WinErr( result != WAIT_FAILED );
          if(result == WAIT_ABANDONED_0)
              warning("TiledAcquisition: Wait 0 abandoned\r\n\t%s\r\n", msg);
          if(result == WAIT_ABANDONED_0+1)
              warning("TiledAcquisition: Wait 1 abandoned\r\n\t%s\r\n", msg);

          if(result == WAIT_TIMEOUT)
              warning("TiledAcquisition: Wait timeout\r\n\t%s\r\n", msg);

          Guarded_Assert_WinErr( result != WAIT_FAILED );

          return -1;
      }

      unsigned int TiledAcquisition::run(device::Microscope *dc)
      { 
        std::string filename;
        unsigned int eflag = 0; // success
        Vector3f tilepos;
        
        Guarded_Assert(dc->__scan_agent.is_runnable());
        //Guarded_Assert(dc->__io_agent.is_running());

        filename = dc->stack_filename(); 
        
        dc->file_series.ensurePathExists();
        eflag |= dc->disk.open(filename,"w");
        if(eflag)
          return eflag;

        eflag |= dc->runPipeline();
        eflag |= dc->__scan_agent.run() != 1;

        StageTiling& tiling = dc->stage()->tiling();
        while(tiling.nextInPlanePosition(tilepos))
        { HANDLE hs[] = {dc->__scan_agent._thread,          
                         dc->__self_agent._notify_stop};
          DWORD res;
          int   t;

          // Move stage
          dc->stage()->setPos(tilepos);

          // wait for scan to complete (or cancel)
          res = WaitForMultipleObjects(2,hs,FALSE,INFINITE);
          t = _handle_wait_for_result(res,"TiledAcquisition::run - Wait for scanner to finish.");
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
          
          tiling.markDone(eflag==0);
          //dc->connect(&dc->disk,0,dc->pipelineEnd(),0);
          
        }
        return eflag;
      }

    } // namespace microscope

  }   // namespace task
}     // namespace fetch
