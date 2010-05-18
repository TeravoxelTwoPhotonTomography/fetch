/*
 * ResonantWrap.cpp
 *
 *  Created on: May 17, 2010
 *      Author: Nathan
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#include "stdafx.h"
#include "ResonantWrap.h"
#include "../util/util-wrap.h"

#if 0
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG
#endif

using namespace fetch::worker;
using namespace resonant_correction::wrap;

namespace fetch
{

  namespace task
  {

    unsigned int
    ResonantWrap::work(Agent *rwa, Frame *fdst, Frame *fsrc)
    { ResonantWrapAgent *agent = dynamic_cast<ResonantWrapAgent*>(rwa);
      float turn;
      int ow,oh,iw,ih;

      turn = agent->config;
      iw = fsrc->width;
      ih = fsrc->height;

      if(!get_dim(iw,ih,turn,&ow,&oh) || ow<0.1*iw) // output width must be >= 10% of input width
      { // turn parameter out-of-bounds
        Frame::copy_data(fdst,fsrc);     // just pass the data through - if fsrc and fdst were pointers to pointers we could avoid this copy
        warning("task: ResonantWrap: turn parameter out of bounds.  turn = %f\r\n",turn);
        agent->SetIsInBounds(false);
        return 1; //return success - [ ] what happens if I return fail here?
      } else
      { agent->SetIsInBounds(true);
      }


      fdst->width =ow;
      fdst->height=oh;
      DBG("In ResonantWrap::work.\r\n");

      //fsrc->dump("ResonantWrap-src.%s",TypeStrFromID(fsrc->rtti));
      switch(fsrc->rtti)
      {
        case id_u8 : transform<u8 >((u8 *)fdst->data,(u8 *)fsrc->data,iw,ih,turn); break;
        case id_u16: transform<u16>((u16*)fdst->data,(u16*)fsrc->data,iw,ih,turn); break;
        case id_u32: transform<u32>((u32*)fdst->data,(u32*)fsrc->data,iw,ih,turn); break;
        case id_u64: transform<u64>((u64*)fdst->data,(u64*)fsrc->data,iw,ih,turn); break;
        case id_i8 : transform<i8 >((i8 *)fdst->data,(i8 *)fsrc->data,iw,ih,turn); break;
        case id_i16: transform<i16>((i16*)fdst->data,(i16*)fsrc->data,iw,ih,turn); break;
        case id_i32: transform<i32>((i32*)fdst->data,(i32*)fsrc->data,iw,ih,turn); break;
        case id_i64: transform<i64>((i64*)fdst->data,(i64*)fsrc->data,iw,ih,turn); break;
        case id_f32: transform<f32>((f32*)fdst->data,(f32*)fsrc->data,iw,ih,turn); break;
        default:
          error("Unrecognized source type (id=%d).\r\n",fsrc->rtti);
      }
      //fdst->dump("ResonantWrap-dst.%s",TypeStrFromID(fdst->rtti));
      return 1; // success

    }

  } // end namespace task

  namespace worker {

    ResonantWrapAgent::ResonantWrapAgent()
    : notify_out_of_bounds_update(INVALID_HANDLE_VALUE),
      is_in_bounds(false)
    { Guarded_Assert_WinErr(
        InitializeCriticalSectionAndSpinCount(&local_state_lock, 0x80000400 ));
      Guarded_Assert_WinErr(
        notify_out_of_bounds_update 
          = CreateEvent(NULL, /*sec attr*/
                        TRUE, /*?manual reset*/
                        TRUE, /*?initial state - default oob*/
                        NULL) /*name*/);
    }

    ResonantWrapAgent::~ResonantWrapAgent()
    { Guarded_Assert_WinErr__NoPanic(CloseHandle(notify_out_of_bounds_update));
      DeleteCriticalSection(&local_state_lock);
    }

    inline bool
    ResonantWrapAgent::Is_In_Bounds(void)
    { return is_in_bounds;
    }

    void
    ResonantWrapAgent::SetIsInBounds(bool val)
    { //only one thread writes is_in_bounds
      //several may read
      if(val==is_in_bounds) return;

      is_in_bounds=val;
      SetEvent(notify_out_of_bounds_update);
    }

    bool
    ResonantWrapAgent::WaitForOOBUpdate(DWORD timeout_ms)
    { HANDLE hs[] = {this->notify_out_of_bounds_update,
                     this->notify_stop};
      DWORD res;
      ResetEvent(notify_out_of_bounds_update);
      res = WaitForMultipleObjects(2,hs,FALSE,timeout_ms);
      return res == WAIT_OBJECT_0;
    }

    int
    ResonantWrapAgent::Set_Turn(float turn, int time)
    { static int lasttime = 0;                     // changes occur inside the lock

      EnterCriticalSection(&local_state_lock);

      if( (time - lasttime) > 0 )                  // The <time> is used to synchronize "simultaneous" requests
      { lasttime = time;                           // Only process requests dated after the last request.

        config = turn;
        // if the device is armed, we need to communicate the change to the bound task
        if(this->is_armed())
        { int run = this->is_running();
          if(run)
            this->stop(RESONANTWRAPAGENT_DEFAULT_TIMEOUT);
          //dynamic_cast<fetch::IUpdateable*>(this->task)->update(this); //commit
          debug("\tChanged Resonant wrap turn set to %f.\r\n",turn);
          if( run )
            this->run();
        }

      }
      LeaveCriticalSection(&local_state_lock);
      return lasttime;
    }


    DWORD WINAPI
    _resonant_wrap_set_turn_val_thread_proc( LPVOID lparam )
    { struct T {ResonantWrapAgent *self; float turn; int time;};
      struct T v = {NULL, 0.0, 0};
      asynq *q = (asynq*) lparam;

      if(!Asynq_Pop_Copy_Try(q,&v,sizeof(T)))
      { warning("In ResonantWrapAgent::Set_Turn work procedure:\r\n"
                "\tCould not pop arguments from queue.\r\n");
        return 0;
      }
      debug( "De-queued request:  ResonantWrapAgent(%p): %f V\t Timestamp: %d\tQ capacity: %d\r\n",v.self, v.turn, v.time, q->q->ring->nelem );
      Guarded_Assert(v.self);
      v.self->Set_Turn(v.turn,v.time);
      return 0; // success
    }

    bool
    ResonantWrapAgent::Set_Turn_NonBlocking(float turn)
    { struct T {ResonantWrapAgent *self; float turn; int time;};
      struct T v = {this, turn, 0};
      static asynq *q = NULL;
      static int timestamp = 0;

      v.time = ++timestamp;

      if( !q )
        q = Asynq_Alloc(2, sizeof(struct T) );
      if( !Asynq_Push_Copy(q, &v, sizeof(T), TRUE /*expand queue when full*/) )
      { warning("In ResonantWrapAgent::Set_Turn_NonBlocking: Could not push request arguments to queue.");
        return 0;
      }
      return QueueUserWorkItem(&_resonant_wrap_set_turn_val_thread_proc, (void*)q, NULL /*default flags*/) == TRUE;
    }
  }  // namespace worker

}
