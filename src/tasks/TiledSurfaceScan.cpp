/**
  \file
  Microscope task.  Acquire one point from analog in for each marked tile in a plane.

  \author Nathan Clack <clackn@janelia.hhmi.org>

  \copyright
  Copyright 2013 Howard Hughes Medical Institute.
  All rights reserved.
  Use is subject to Janelia Farm Research Campus Software Copyright 1.1
  license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

/*
PARAMETERS
- output file
  - save x y and measured voltage one line at a time in a csv
- maybe needs to know the ai terminal to use for the daq, though this should probably be abstracted through a device.

TILING
- Use tiling for stage positions.  Iterate over in plane explorable till done with plane.

DEVICE
- a DAQ analog in device.  No task or agent necessary.  Should make sure scan agent isn't armed, but I think that's gauranteed by the scope agent.
  - just needs a synchronous "float read();" method
 */


#include "common.h"
#include "TiledSurfaceScan.h"
#include "StackAcquisition.h"
#include "Video.h"
#include "frame.h"
#include "devices\digitizer.h"
#include "devices\Microscope.h"
#include "devices\tiling.h"

#if 0 // PROFILING
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
    // TiledSurfacescan -  microscope task
    //

    namespace microscope {

      //Upcasting
      unsigned int TiledSurfacescan::config(IDevice *d) {return config(dynamic_cast<device::Microscope*>(d));}
      unsigned int TiledSurfacescan::run   (IDevice *d) {return run   (dynamic_cast<device::Microscope*>(d));}

      unsigned int TiledSurfacescan::config(device::Microscope *dc)
      { unsigned int isok=1;
        // -- validate input parameters --
        // 1. ensure output file is writable.
        // 2. ensure daq is available for read on request.
        CHKJMP(!dc->__scan_agent.is_runnable()); // (maybe)
        return isok; //success
Error:
        return 0;
      }

      unsigned int TiledSurfacescan::run(device::Microscope *dc)
      {
        unsigned int eflag = 0; // success
        FILE *fp=0;
        Vector3f tilepos;
        TS_OPEN("timer-probe.f32");
        
        CHKJMP(fp=fopen("surface_scan.csv","w"));
        fprintf(fp,"stagex\tstagey\tstagez\tprobe_mV\n");

        device::StageTiling* tiling = dc->stage()->tiling();
        tiling->resetCursor();

        while(eflag==0 && !dc->_agent->is_stopping() && tiling->nextInPlaneExplorablePosition(tilepos))
        { TS_TIC;
          debug("%s(%d)"ENDL "\t[Surface Scan Task] tilepos: %5.1f %5.1f %5.1f"ENDL,__FILE__,__LINE__,tilepos[0],tilepos[1],tilepos[2]);

          // Move stage
          Vector3f curpos = dc->stage()->getTarget(); // use current target z for tilepos z
          tilepos[2] = curpos[2]*1000.0f;             // unit conversion here is a bit awkward
          dc->stage()->setPos(0.001f*tilepos);        // convert um to mm

          float x,y,z,v=dc->surfaceProbe()->read();
          dc->stage()->getPos(&x,&y,&z);
          fprintf(fp,"%f\t%f\t%f\t%f\n",x,y,z,v);

          TS_TOC;
        } // end loop over tiles
Finalize:
/*TODO*/// Close output file.
        if(fp) fclose(fp);
        TS_CLOSE;
        return eflag;
Error:
        eflag=1;
        goto Finalize;
      }

    } // namespace microscope

  }   // namespace task
}     // namespace fetch
