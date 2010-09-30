/*
 * Video.h
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#pragma once
#include "../task.h"
#include "../devices/scanner2D.h"
#include "../agent.h"

namespace fetch
{

  namespace task
  {
    namespace scanner
    {
      template<class TPixel>
      class Video : public Task, public IUpdateable
      {
        public:
          unsigned int config (Agent *d);
          unsigned int run    (Agent *d);
          unsigned int update (Agent *d);
          
          unsigned int config (device::Scanner2D *d);
          unsigned int run    (device::Scanner2D *d);
          unsigned int update (device::Scanner2D *d);
      };

    }
  }
}
