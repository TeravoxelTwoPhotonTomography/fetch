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
  class WorkAgent : public Agent
  { public:
      WorkAgent();                                           // Will configure only.  Use apply() to connect, arm, and run.  config is set to TParam().
      WorkAgent(TParam parameter);                           // Will configure only.  Use apply() to connect, arm, and run.
      WorkAgent(Agent *source, int ichan, TParam parameter); // Will connect, configure, arm, and run
      ~WorkAgent();

      WorkAgent<TWorkTask,TParam>* apply(Agent *source, int ichan=0); // returns <this>

      unsigned int attach(void);
      unsigned int detach(void);

    public: //data
      TParam    config;
      TWorkTask __task_instance;
  };

  ////////////////////////////////////////////////////////////////////////////
  //
  // Implementation
  //
  ////////////////////////////////////////////////////////////////////////////
  
  template<typename TWorkTask,typename TParam>
  WorkAgent<TWorkTask,TParam>::WorkAgent()
  : config(),
    __task_instance()
  { attach(); }

  template<typename TWorkTask,typename TParam>
  WorkAgent<TWorkTask,TParam>::WorkAgent(TParam parameter)
  : config(parameter),
    __task_instance()
  { attach(); }

  template<typename TWorkTask,typename TParam>
  WorkAgent<TWorkTask,TParam>::WorkAgent(Agent *source, int ichan, TParam parameter)
  { config = parameter;
    __task_instance = TWorkTask();            // a bit kludgey - the WorkAgent is only ever associated with a single task.
    apply(source,ichan);
  }
  
  template<typename TWorkTask,typename TParam>
  WorkAgent<TWorkTask,TParam>::~WorkAgent(void)
  { if(this->detach()>0)
      warning("Could not cleanly detach WorkAgent (task instance at 0x%p).\r\n",&this->__task_instance);
  }
  
  template<typename TWorkTask,typename TParam>
  unsigned int WorkAgent<TWorkTask,TParam>::
  attach(void) 
  { this->lock(); 
    this->set_available(); 
    this->unlock(); 
    return 0; /*0 success, 1 failure*/
  }
  
  template<typename TWorkTask,typename TParam>
  unsigned int WorkAgent<TWorkTask,TParam>::
  detach(void) 
  { debug("Attempting WorkAgent::disarm() for task at 0x%p.\r\n",&this->__task_instance);
    if( !this->disarm(WORKER_DEFAULT_TIMEOUT) )
      warning("Could not cleanly detach WorkAgent (task instance at 0x%p).\r\n",&this->__task_instance);  
      
    this->lock();
    this->_is_available=0;
    this->unlock();
    return 0; /*0 success, 1 failure*/
  }
  
  template<typename TWorkTask,typename TParam>
  WorkAgent<TWorkTask,TParam>*
  WorkAgent<TWorkTask,TParam>::apply(Agent *source, int ichan)
  { connect(this,ichan,source,ichan);
    if( out==NULL )
      __task_instance.alloc_output_queues(this);// Task must implement this.  Must connect() first.  WorkTask has a default impl. that assumes in[0]->out[0].  These should handle pre-existing queues (by freecycling).
    Guarded_Assert( disarm(WORKER_DEFAULT_TIMEOUT));
    Guarded_Assert( arm(&__task_instance,WORKER_DEFAULT_TIMEOUT));
    Guarded_Assert( run());
    return this;
  }

}
