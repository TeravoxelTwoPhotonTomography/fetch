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
#include "../task.h"
#include "../devices/microscope.h"

namespace fetch
{ namespace task
  { namespace microscope
    {
      typedef UpcastTask<device::Microscope> MicroscopeTask;
      
      class Interaction : public MicroscopeTask
      { public:
          unsigned int config(device::Microscope *agent);
          unsigned int    run(device::Microscope *agent);
      };
      
    }
  }
}