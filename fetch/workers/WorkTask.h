/*
 * WorkTask.h
 *
 *  Created on: Apr 22, 2010
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
#include "frame.h"

namespace fetch
{

  namespace task
  {

    class WorkTask : public fetch::Task
    { public:
        unsigned int config(Agent *d) {}
    };

    class UpdateableWorkTask : public fetch::UpdateableTask
    { public:
        unsigned int config(Agent *d) {}
    };

    class OneToOneWorkTask : public WorkTask
    { public:
                unsigned int run(Agent *d);
        virtual unsigned int work(Message *dst, Message *src);
    };

  }

}
