/*
 * NIDAQAgent.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: clackn
 */

#include "stdafx.h"
#include "NIDAQAgent.h"

#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  namespace device
  {

    NIDAQAgent::NIDAQAgent(Agent *agent, char *name)
      :IConfigurableDevice<char*>(agent,name)
      ,daqtask(NULL)
    { 
      /*
      TODO: _daqtaskname and _config are redundant (I think).
      XXX:  may not need _daqtaskname anymore
      */

      // Copy name
      size_t n = strlen(name);
      Guarded_Assert(n<sizeof(_daqtaskname));
      memset(_daqtaskname,0,sizeof(_daqtaskname));
      memcpy(_daqtaskname,name,n);
    }
    
    NIDAQAgent::~NIDAQAgent(void)
    { if(this->detach()>0)
        warning("Couldn't cleanly detach NIDAQAgent: %s\r\n.",this->_daqtaskname);
    }

    unsigned int NIDAQAgent::detach(void)
    { unsigned int status=1; //success 0, failure 1

      if(daqtask)
      { debug("%s: Attempting to detach DAQ AO channel. handle 0x%p\r\n",_daqtaskname,daqtask);
        DAQJMP(DAQmxStopTask(daqtask));
        DAQJMP(DAQmxClearTask(daqtask));
      }
      status = 0;
    Error:
      daqtask = NULL;
      return status;
    }

    unsigned int NIDAQAgent::attach(void)
    { unsigned int status = 1; //success 0, failure 1;
      Guarded_Assert(daqtask==NULL);
      DAQJMP(status=DAQmxCreateTask(_daqtaskname,&daqtask))
      status = 0;
    Error:
      return status;
    }
    
  }

}
