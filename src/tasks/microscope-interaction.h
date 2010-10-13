/*
 * microscope-interaction.h
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
#include "task.h"
#include "devices\Microscope.h"

namespace fetch
{ 
  namespace device
  {
    class Microscope;
  }

  namespace task
  { namespace microscope
    {
      //typedef UpcastTask<device::Microscope> MicroscopeTask;
      typedef Task MicroscopeTask;
      
      class Interaction : public MicroscopeTask
      { public:
          unsigned int config(IDevice *d);
          unsigned int    run(IDevice *d);
          
          unsigned int config(device::Microscope *agent);
          unsigned int    run(device::Microscope *agent);
      };
      
    }
  }
}