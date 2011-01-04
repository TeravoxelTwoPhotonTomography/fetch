/*
 * StackAcquisition.h
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

#pragma once

#include "task.h"

namespace fetch {
namespace device {
  class Scanner3D;
  class Microscope;
}
}

// #include "devices\Scanner3d.h"
// #include "devices\Microscope.h"

#define SCANNER_STACKACQ_TASK_FETCH_TIMEOUT 15.0

namespace fetch
{

  namespace task
  {
    namespace microscope {

      typedef Task MicroscopeTask;

      class StackAcquisition : public MicroscopeTask
      {
        public:
          unsigned int config(IDevice *d);
          unsigned int    run(IDevice *d);

          unsigned int config(device::Microscope *agent);
          unsigned int    run(device::Microscope *agent);
      };

    }  // namespace microscope

    namespace scanner
    {
      template<class TPixel>
      class ScanStack : public fetch::Task, public fetch::IUpdateable
      {
      public:
        unsigned int config (IDevice *d);
        unsigned int run    (IDevice *d);
        unsigned int update (IDevice *d);

        unsigned int config (device::Scanner3D *d);        
        unsigned int update (device::Scanner3D *d);

        unsigned int run_niscope   (device::Scanner3D *d);
        unsigned int run_alazar    (device::Scanner3D *d);
        unsigned int run_simulated (device::Scanner3D *d);
      };


    }  // namespace scanner

  }

}
