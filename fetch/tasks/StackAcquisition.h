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

#include "../stdafx.h"
#include "../task.h"
#include "../devices/Scanner3d.h"

#define SCANNER_STACKACQ_TASK_FETCH_TIMEOUT INFINITE

namespace fetch
{

  namespace task
  {

    namespace scanner
    {
      template<class TPixel>
      class ScanStack : public fetch::Task, public fetch::IUpdateable
      {
      public:
        unsigned int config (Agent *d);
        unsigned int run    (Agent *d);
        unsigned int update (Agent *d);

        unsigned int config (device::Scanner3D *d);
        unsigned int run    (device::Scanner3D *d);
        unsigned int update (device::Scanner3D *d);
      };

    }

  }

}
