/*
 * WorkAgent.h
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
/*
 * WorkAgent
 * ---------
 *
 * Notes
 * -----
 * Privately inherits Agent.  The idea is that state manipulation is delegated
 * to construction/destruction.  There doesn't need to be any outside access
 * to state manipulation functions.
 *
 *
 */
#ifndef WORKAGENT_H_
#define WORKAGENT_H_

#include "agent.h"

namespace fetch
{
  /*
   * Example:
   *
   * device::Scanner2D src;
   * WorkAgent<task::FrameAverager,int> step1(&src,  // producer Agent
   *                                             0,  // x so that step1->in[0] is src->out[x]
   *                                             4); // times to average
   */
  template<typename TWorkTask, typename TParam>
  class WorkAgent : private virtual fetch::Agent
  { public:
      WorkAgent(Agent *source, int ichan, TParam parameter);

      unsigned int attach(Agent *a) {return 1/*success*/;}
      unsigned int detach(Agent *a) {return 1/*success*/;}

    private: //data
      TParam    config;
      TWorkTask __task_instance;
  };

}

//
// Implementation
//
namespace fetch
{
  template<typename TWorkTask,typename TParam>
  WorkAgent::WorkAgent(Agent *source, int ichan, TParam parameter)
  { config = parameter;
    connect(this,ichan,source,ichan);
    __task_instance = TWorkTask();       // a bit kludgey - the WorkAgent is only ever associated with a single task.
    arm(&task_instance,INFINITE);
    run();
  }

}
