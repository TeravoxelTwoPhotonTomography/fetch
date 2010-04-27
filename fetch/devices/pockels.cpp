/*
 * Pockels.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 *//*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#include "stdafx.h"
#include "Pockels.h"

#define DAQWRN( expr )        (Guarded_DAQmx( (expr), #expr, warning))
#define DAQERR( expr )        (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )        goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  namespace device
  {

    Pockels::Pockels(void)
            : NIDAQAgent("Pockels"),
              config(POCKELS_CONFIG_DEFAULT),
              daq_ao(NULL),
              local_state_lock(NULL)
    { Guarded_Assert_WinErr(
        InitializeCriticalSectionAndSpinCount(&local_state_lock, 0x80000400 ));
    }

    Pockels::~Pockels()
    { DeleteCriticalSection(&local_state_lock);
    }

    int
    Pockels::Is_Volts_In_Bounds(f64 volts)
    { return ( volts >= config.v_lim_min ) && (volts <= config.v_lim_max );
    }

    int
    Pockels::Set_Open_Val(f64 volts, int time)
    { static int lasttime = 0;                     // changes occur inside the lock
      Guarded_Assert( Is_Volts_In_Bounds(volts) ); // Callers that don't like the panics (e.g. UI responses)
                                                   //   should do their own bounds checks.
      EnterCriticalSection(&local_state_lock);

      if( (time - lasttime) > 0 )                  // The <time> is used to synchronize "simultaneous" requests
      { lasttime = time;                           // Only process requests dated after the last request.

        config.v_open = volts;
        // if the device is armed, we need to communicate the change to the bound task
        if(this->is_armed())
        { int run = this->is_running();
          if(run)
            this->stop(POCKELS_DEFAULT_TIMEOUT);
          dynamic_cast<UpdateableTask*>(this->task)->update(this); //commit
          debug("\tChanged Pockels to %f V at open.\r\n",volts);
          if( run )
            this->run();
        }

      }
      LeaveCriticalSection(&local_state_lock);
      return lasttime;
    }

#ifndef WINAPI
#define WINAPI
#endif

    DWORD WINAPI
    _pockels_set_open_val_thread_proc( LPVOID lparam )
    { struct T {Pockels *self; f64 volts; int time;};
      struct T v = {NULL, 0.0, 0};
      asynq *q = (asynq*) lparam;

      if(!Asynq_Pop_Copy_Try(q,&v))
      { warning("In Pockels::Set_Open_Val work procedure:\r\n"
                "\tCould not pop arguments from queue.\r\n");
        return 0;
      }
      debug( "De-queued request:  Pockels(%p): %f V\t Timestamp: %d\tQ capacity: %d\r\n",v.self, v.volts, v.time, q->q->ring->nelem );
      Guarded_Assert(v.self);
      v.self->Set_Open_Val(v.volts,v.time);
      return 0; // success
    }

    BOOL
    Pockels::Set_Open_Val_Nonblocking(f64 volts)
    { struct T {Pockels *self; f64 volts; int time;};
      struct T v = {this, volts, 0};
      static asynq *q = NULL;
      static int timestamp = 0;

      v.time = ++timestamp;

      if( !q )
        q = Asynq_Alloc(2, sizeof(struct T) );
      if( !Asynq_Push_Copy(q, &v, TRUE /*expand queue when full*/) )
      { warning("In Scanner_Pockels_Set_Open_Val_Nonblocking: Could not push request arguments to queue.");
        return 0;
      }
      return QueueUserWorkItem(&_pockels_set_open_val_thread_proc, (void*)q, NULL /*default flags*/);
    }


  } //end device namespace
}   //end fetch  namespace
