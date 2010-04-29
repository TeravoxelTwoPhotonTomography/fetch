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

    NIDAQAgent::NIDAQAgent(char *name)
               : Agent(),daqtask(NULL)
    { // Copy name
      size_t n = strlen(name);
      Guarded_Assert(n<sizeof(_daqtaskname));
      memset(_daqtaskname,0,sizeof(_daqtaskname));
      memcpy(_daqtaskname,name,n);
    }

    unsigned int NIDAQAgent::detach(void)
    { unsigned int status=1; //success 0, failure 1

      if( !this->disarm(NIDAQAGENT_DEFAULT_TIMEOUT) )
        warning("Could not cleanly disarm NIDAQAgent: %s\r\n",_daqtaskname);

      if(daqtask)
      { debug("%s: Attempting to detach DAQ AO channel. handle 0x%p\r\n",_daqtaskname,daqtask);
        DAQJMP(DAQmxStopTask(daqtask));
        DAQJMP(DAQmxClearTask(daqtask));
      }
    Error:
      daqtask = NULL;
      this->_is_available=0;
      this->unlock();
      return status;
    }

    unsigned int NIDAQAgent::attach(void)
    { unsigned int status = 0; //success 0, failure 1;
      Guarded_Assert(daqtask==NULL);
      this->lock();
      DAQJMP(status=DAQmxCreateTask(_daqtaskname,&daqtask))
      this->set_available();
    Error:
      this->unlock();
      return status;
    }
    
  }

}
