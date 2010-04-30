/*
 * Microscope.h
 *
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 28, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once

#include "../agent.h"
#include "../task.h"
#include "scanner2D.h"

#include "../workers/FrameAverage.h"
#include "../workers/PixelWiseAverager.h"
#include "../workers/Terminator.h"

#include "DiskStream.h"

#define MICROSCOPE_MAX_WORKERS     10
#define MICROSCOPE_DEFAULT_TIMEOUT INFINITE

namespace fetch
{ namespace device
  {
  
    class Microscope : public virtual Agent
    { public:
                        Microscope();
        virtual inline ~Microscope();
        
        unsigned int attach(void);
        unsigned int detach(void);
        
        unsigned int disarm(DWORD timeout_ms);
        
      public:
        device::Scanner2D              scanner;

        worker::FrameAverageAgent 	   frame_averager;
        worker::PixelWiseAveragerAgent pixel_averager;
        worker::TerminalAgent		       trash;

        device::DiskStreamMessage      disk;
    };
    
  }
}
