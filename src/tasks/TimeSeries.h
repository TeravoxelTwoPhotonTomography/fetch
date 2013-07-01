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

#pragma once

#include "task.h"

namespace fetch {
namespace device {
  class Scanner3D;
  class Microscope;
}
}

namespace fetch
{

  namespace task
  {
    namespace microscope {

      typedef Task MicroscopeTask;

      class TimeSeries : public MicroscopeTask
      {
        public:
          unsigned int config(IDevice *d);
          unsigned int    run(IDevice *d);

          unsigned int config(device::Microscope *agent);
          unsigned int    run(device::Microscope *agent);
      };

    }  // namespace microscope

  }
}
