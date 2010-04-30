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
 * WorkAgent<TWorkTask,TParam=void*>
 * ---------------------------------
 *
 * TWorkTask
 *  - must be a child of class Task.
 *  - must implement alloc_output_queues(Agent*)
 *
 * Notes
 * -----
 * Privately inherits Agent.  The idea is that state manipulation is delegated
 * to construction/destruction.  There doesn't need to be any outside access
 * to state manipulation functions.
 *
 *
 */
#pragma once

#include "../agent.h"

#define WORKER_DEFAULT_TIMEOUT INFINITE

namespace fetch
{
  /*
   * Example:
   *
   * device::Scanner2D src;
   * WorkAgent<task::FrameAverager,int> step1(&src,  // producer Agent
   *                                             0,  // x so that step1->in[0] is src->out[x]
   *                                             4); // times to average
   *
   *
   * ...or...
   *
   * device::Scanner2D src;
   * WorkAgent<task::FrameAverager,int> step1(2);     // times to average   
   * WorkAgent<task::PixelWiseAverager,int> step2(4); // times to average
   * Agent *last = step2.apply(step1.apply(&src,0));
   */
  template<typename TWorkTask, typename TParam=void*>
  class WorkAgent : public virtual fetch::Agent
  { public:
      WorkAgent();                                           // Will configure only.  Use apply() to connect, arm, and run.  config is set to TParam().
      WorkAgent(TParam parameter);                           // Will configure only.  Use apply() to connect, arm, and run.
      WorkAgent(Agent *source, int ichan, TParam parameter); // Will connect, configure, arm, and run

      WorkAgent<TWorkTask,TParam>* apply(Agent *source, int ichan=0); // returns <this>

      unsigned int attach(void) {return 1/*success*/;}
      unsigned int detach(void) {return 1/*success*/;}

    public: //data
      TParam    config;
      TWorkTask __task_instance;
  };

  //
  // Implementation
  //

  template<typename TWorkTask,typename TParam>
  WorkAgent<TWorkTask,TParam>::WorkAgent()
  : config(),
    __task_instance()
  { }

  template<typename TWorkTask,typename TParam>
  WorkAgent<TWorkTask,TParam>::WorkAgent(TParam parameter)
  : config(parameter),
    __task_instance()
  { }

  template<typename TWorkTask,typename TParam>
  WorkAgent<TWorkTask,TParam>::WorkAgent(Agent *source, int ichan, TParam parameter)
  { config = parameter;
    connect(this,ichan,source,ichan);
    TWorkTask::alloc_output_queues(this);// Task must implement this.  Must connect() first.  WorkTask has a default impl. that assumes in[0]->out[0]
    __task_instance = TWorkTask();       // a bit kludgey - the WorkAgent is only ever associated with a single task.
    arm(&task_instance,INFINITE);
    run();
  }
  
  template<typename TWorkTask,typename TParam>
  WorkAgent<TWorkTask,TParam>*
  WorkAgent<TWorkTask,TParam>::apply(Agent *source, int ichan)
  { connect(this,ichan,source,ichan);
    TWorkTask::alloc_output_queues(this);// Task must implement this.  Must connect() first.  WorkTask has a default impl. that assumes in[0]->out[0]
    arm(&task_instance,INFINITE);
    run();
    return this;
  }

}
