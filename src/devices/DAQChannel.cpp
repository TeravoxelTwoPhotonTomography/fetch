/*
 * NIDAQChannel.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: clackn
 */


#include "DAQChannel.h"

#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  namespace device
  {

    NIDAQChannel::NIDAQChannel(Agent *agent, char *name)
      :IConfigurableDevice<char*>(agent,&name)
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
    
    NIDAQChannel::~NIDAQChannel(void)
    { if(this->on_detach()>0)
        warning("Couldn't cleanly detach NIDAQChannel: %s\r\n.",this->_daqtaskname);
    }

    unsigned int NIDAQChannel::on_detach(void)
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

    unsigned int NIDAQChannel::on_attach(void)
    { unsigned int status = 1; //success 0, failure 1;
      Guarded_Assert(daqtask==NULL);
      HERE;
      debug("\t%s"ENDL,_daqtaskname);
      DAQJMP(status=DAQmxCreateTask(_daqtaskname,&daqtask))
      status = 0;
    Error:
      return status;
    }
    

    SimulatedDAQChannel::SimulatedDAQChannel( Agent *agent, char *name )
      :IConfigurableDevice<char*>(agent,&name)      
    {
      memset(_name,0,sizeof(_name));
      memcpy(_name,name,MIN(strlen(name),strlen(_name)));
    }

    unsigned int SimulatedDAQChannel::on_attach()
    {
      return 0;
    }

    unsigned int SimulatedDAQChannel::on_detach()
    {
      return 0;
    }

  }

}
