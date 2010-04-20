/*
 * Shutter.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: clackn
 */

#include "stdafx.h"
#include "Shutter.h"

#include "util-nidaqmx.h"

#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  namespace device
  {

    Shutter::Shutter()
            : config((Shutter::Config)SHUTTER_DEFAULT_CONFIG),
              dout(NULL)
    {
      // TODO Auto-generated constructor stub

    }

    Shutter::~Shutter()
    {
      // TODO Auto-generated destructor stub
    }

    /*
     * Note:
     * ----
     * Another way of doing this would be to follow the pattern used by the
     * Pockels class.  That is, Set() makes a call to the Task to commit the
     * new value to hardware while appropriately controlling Agent state.
     *
     * The approach used below doesn't use any state management or
     * synchronization.  As a result the implementation is simple, but there's
     * very little control over when these changes are executed.
     *
     * However, this class is used in a pretty specific context.  So far, this
     * simple implementation has been enough.
     */
    void
    Shutter::Set(u8 val)
    { int32 written = 0;
      DAQERR( DAQmxWriteDigitalLines( this->daqtask,
                                      1,                          // samples per channel,
                                      0,                          // autostart
                                      0,                          // timeout
                                      DAQmx_Val_GroupByChannel,   // data layout,
                                      &val,                       // buffer
                                      &written,                   // (out) samples written
                                      NULL ));                    // reserved
    }

    void
    Shutter::Open(void)
    { Set(config.open);
    }

    void
    Shutter::Close(void)
    { Set(config.closed);
    }

  }

}
