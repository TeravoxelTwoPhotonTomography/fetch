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
#include "devices\tiling.h"

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

        d->__scan_agent.arm(&grabstack,&d->scanner);                       // why was this arm_nowait?     - should prolly move this to run

        d->stage()->tiling()->resetCursor();                               // this is here so that run/stop cycles will pick up where they left off

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

        device::StageTiling* tiling = dc->stage()->tiling();
        while(!dc->_agent->is_stopping() && tiling->nextInPlanePosition(tilepos))
        {
          debug("[Tiling Task] tilepos: %5.1f %5.1f %5.1f"ENDL,tilepos[0],tilepos[1],tilepos[2]);
          filename = dc->stack_filename(); 
          dc->file_series.ensurePathExists();
          eflag |= dc->disk.open(filename,"w");
          if(eflag)
          { 
            warning("Couldn't open file: %s"ENDL, filename.c_str());
            //tiling->markDone(eflag==0);
            return eflag;
          }

          // Move stage
          Vector3f curpos = dc->stage()->getPos(); // FIXME HACK
          tilepos[2] = curpos[2]*1000.0f;
          
          dc->stage()->setPos(0.001f*tilepos); // convert um to mm

          eflag |= dc->runPipeline();
          eflag |= dc->__scan_agent.run() != 1;          

          {
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
            case 0:       // in this case, the scanner thread stopped.  Nothing left to do.
              eflag |= 0; // success
              tiling->markDone(eflag==0); // only mark the tile done if the scanner task completed
              //break; 
            case 1:       // in this case, the stop event triggered and must be propagated.
              eflag |= dc->__scan_agent.stop(SCANNER2D_DEFAULT_TIMEOUT) != 1;
              break;
            default:      // in this case, there was a timeout or abandoned wait
              eflag |= 1; //failure              
            }
          }  // end waiting block

          // Output metadata and Increment file
          eflag |= dc->disk.close();
          dc->write_stack_metadata();
          dc->file_series.inc(); // increment regardless of completion status

          eflag |= dc->stopPipeline(); // wait till everything stops
          tiling->resetCursor();
          
        } // end loop over tiles
        eflag |= dc->stopPipeline(); // wait till the  pipeline stops
        
        return eflag;
      }

    } // namespace microscope

  }   // namespace task
}     // namespace fetch
