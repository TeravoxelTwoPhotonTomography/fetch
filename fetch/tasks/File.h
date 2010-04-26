/*
 * File.h
 *
 *  Created on: Apr 23, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once

#include "../devices/DiskStream.h"
#include "task.h"

namespace fetch
{

  namespace task
  {
    namespace file {

      class WriteRaw : public fetch::Task
      { public:
          unsigned int config(device::DiskStream *agent);
          unsigned int run(device::DiskStream *agent);
      };

      class ReadRaw : public fetch::Task
      { public:
          unsigned int config(device::DiskStream *agent);
          unsigned int run(device::DiskStream *agent);
      };

      class WriteMessage : public fetch::Task
      { public:
          unsigned int config(device::DiskStream *agent);
          unsigned int run(device::DiskStream *agent);
      };

      class ReadMessage : public fetch::Task
      { public:
          unsigned int config(device::DiskStream *agent);
          unsigned int run(device::DiskStream *agent);
      };

    }  // namespace disk

  }

}
