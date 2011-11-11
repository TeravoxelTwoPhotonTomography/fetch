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
 * WorkAgent<TWorkTask,TConfig=void*>
 * ---------------------------------
 *
 * TWorkTask
 *  - must be a child of class Task.
 *  - must implement alloc_output_queues(Agent*)
 *
 * Notes
 * -----
 * Bad name.  Inherits IDevice...should be called a WorkDevice.
 *
 *
 */
#pragma once

#include "agent.h"

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
  template<typename TWorkTask, typename TConfig=void*>
  class WorkAgent : public IConfigurableDevice<TConfig>
  { 
  public:
    typedef TWorkTask TaskType;

    WorkAgent(char* name=NULL);                                              // Will configure only.  Use apply() to connect, arm, and run.  config is set to TConfig().
    WorkAgent(TConfig *config,char* name=NULL);                              // Will configure only.  Use apply() to connect, arm, and run.
    WorkAgent(IDevice *source, int ichan, TConfig *config,char* name=NULL);  // Will connect, configure, arm, and run

    WorkAgent<TWorkTask,TConfig>* apply(IDevice *source, int ichan=0); // returns <this>

    unsigned int on_attach(void);
    unsigned int on_detach(void);

    //virtual void set_config(Config *cfg)        {HERE; IConfigurableDevice<TConfig>::set_config(cfg);}
    //virtual void set_config(const Config &cfg)  {HERE; IConfigurableDevice<TConfig>::set_config(cfg);}

  public: //data
    TWorkTask __task_instance;
    Agent     __agent_instance;
  };

  ////////////////////////////////////////////////////////////////////////////
  //
  // Implementation
  //
  ////////////////////////////////////////////////////////////////////////////
  
  template<typename TWorkTask,typename TConfig>
  WorkAgent<TWorkTask,TConfig>::WorkAgent(char* name)
    :IConfigurableDevice<TConfig>(&__agent_instance)
    ,__agent_instance(name,NULL)    
  {
    __agent_instance._owner = this;
    _agent->attach(); 
  }

  template<typename TWorkTask,typename TConfig>
  WorkAgent<TWorkTask,TConfig>::WorkAgent(TConfig *config,char* name)
    :IConfigurableDevice<TConfig>(&__agent_instance,config)
    ,__agent_instance(name,NULL)
  { 
    __agent_instance._owner = this;
    _agent->attach(); 
  }

  template<typename TWorkTask,typename TConfig>
  WorkAgent<TWorkTask,TConfig>::WorkAgent(IDevice *source, int ichan, TConfig *config,char* name)
    :IConfigurableDevice<TConfig>(&__agent_instance,config)
    ,__agent_instance(name,this)
  { 
    _agent->attach();
    apply(source,ichan);
  }
  
  template<typename TWorkTask,typename TConfig>
  unsigned int WorkAgent<TWorkTask,TConfig>::
  on_attach(void) 
  {
    return 0; /*0 success, 1 failure*/
  }
  
  template<typename TWorkTask,typename TConfig>
  unsigned int WorkAgent<TWorkTask,TConfig>::
  on_detach(void) 
  { debug("Attempting WorkAgent::on_detach() for task at 0x%p [%s].\r\n",&this->__task_instance,_agent->name());    
    return 0; /*0 success, 1 failure*/
  }
  
  template<typename TWorkTask,typename TConfig>
  WorkAgent<TWorkTask,TConfig>*
  WorkAgent<TWorkTask,TConfig>::apply(IDevice *source, int ichan)
  { 
    connect(this,ichan,source,ichan);
    if( _out==NULL )
      __task_instance.alloc_output_queues(this);// Task must implement this.  Must connect() first.  WorkTask has a default impl. that assumes in[0]->out[0].  These should handle pre-existing queues (by freecycling).
    Guarded_Assert(_agent->disarm(WORKER_DEFAULT_TIMEOUT)==0);
    Guarded_Assert(_agent->arm(&__task_instance,this,WORKER_DEFAULT_TIMEOUT)==0);
    //Guarded_Assert(_agent->run());
    return this;
  }

}
