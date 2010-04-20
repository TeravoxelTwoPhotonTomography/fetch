/*
 * NIDAQAgent.h
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
/*
 * Agent's that attach to NI-DAQmx resources through calls to:
 *
 *    DAQmxCreateTask
 *
 * should inherit from this class.  This class provides the attach/detach
 * functions required of the Agent interface.
 */

#pragma once

#include "agent.h"

#define NIDAQAGENT_DEFAULT_TIMEOUT INFINITY

namespace fetch
{

  namespace device
  {

    class NIDAQAgent : public virtual Agent
    {
    public:
      NIDAQAgent(char *name);

      unsigned int attach();
      unsigned int detach();

    public:
      TaskHandle daqtask;

    protected:
      char _daqtaskname[128];
    };

  }

}

