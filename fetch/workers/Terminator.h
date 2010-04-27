/*
 * Terminator.h
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
#include "WorkTask.h"
#include "WorkAgent.h"

namespace fetch
{

  namespace task
  {

    typedef WorkAgent<Terminator> TerminalAgent;

    class Terminator : public WorkTask
    { public:
        unsigned int run(TerminalAgent *agent);
        virtual static void alloc_output_queues(Agent *agent);//noop
    };

  }

}
