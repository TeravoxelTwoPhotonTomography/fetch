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
#include "scanner3D.h"

#include "../workers/FrameAverage.h"
#include "../workers/PixelWiseAverager.h"
#include "../workers/Terminator.h"
#include "../workers/FrameCaster.h"

#include "DiskStream.h"

#define MICROSCOPE_DEFAULT_DEST_PATH           "E:\\microscope\\data\\2010-05-13\\"
#define MICROSCOPE_DEFAULT_FILE_NAME_TEMPLATE  "Stack_%03d.stream"
#define MICROSCOPE_DEFAULT_INITIAL_FILE_ID     0

#define MICROSCOPE_MAX_WORKERS     10
#define MICROSCOPE_DEFAULT_TIMEOUT INFINITE

namespace fetch
{ namespace device
  {
  
    class Microscope : public virtual Agent
    { public:
        Microscope();
        ~Microscope();
        
        unsigned int attach(void);
        unsigned int detach(void);
        
        unsigned int disarm(DWORD timeout_ms);
        
      public:
        void next_filename(char *dest);                // assembles filename and incriments config.file_iter_id
        
      public:
        device::Scanner3D              scanner;

        worker::FrameAverageAgent 	   frame_averager;
        worker::PixelWiseAveragerAgent pixel_averager;        
        worker::FrameCastAgent_i16     cast_to_i16;
        worker::TerminalAgent		       trash;                

        device::DiskStreamMessage      disk;
        
      public:
        struct Config
        { char path[MAX_PATH];
          char filename_template[MAX_PATH];
          u64  file_iter_id;
          Config();                    
        };
        
        Config config;
    };
    
  }
}
