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


#include "ResonantWrap.h"
#include "../util/util-wrap.h"

#if 0
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG
#endif

// I use REMIND to warn me in case I leave in a debugging expression.
#define REMIND(expr) \
  warning("%s(%d)" ENDL "\tEvaluating expression." ENDL "\t%s" ENDL, __FILE__,__LINE__,#expr); \
  (expr)

using namespace fetch::worker;
using namespace resonant_correction::wrap;

namespace fetch
{
  bool operator==(const cfg::worker::ResonantWrap& a, const cfg::worker::ResonantWrap& b)
  { return fabs(a.turn_px()-b.turn_px())<1e-3;
  }
  bool operator!=(const cfg::worker::ResonantWrap& a, const cfg::worker::ResonantWrap& b)
  { return !(a==b);
  }

  namespace task
  {
    unsigned int ResonantWrap::reshape( IDevice *d, Frame *fdst )
    {
      ResonantWrapAgent *dc = dynamic_cast<ResonantWrapAgent*>(d);
      float turn;
      int ow,oh,iw,ih;

      Guarded_Assert(dc);

      turn = dc->get_config().turn_px();
      iw = fdst->width;
      ih = fdst->height;

      if(!get_dim(iw,ih,turn,&ow,&oh) || ow<0.1*iw) // output width must be >= 10% of input width
      {
        // turn parameter out-of-bounds
        dc->setIsInBounds(false);
        return 1; //success
      } else
      {
        dc->setIsInBounds(true);
      }
      fdst->width =ow;
      fdst->height=oh;

      return 1; // success
    }

    unsigned int
    ResonantWrap::work(IDevice *idc, Frame *fdst, Frame *fsrc)
    { ResonantWrapAgent *dc = dynamic_cast<ResonantWrapAgent*>(idc);
      float turn;
      int iw,ih;

      Guarded_Assert(dc);

      turn = dc->get_config().turn_px();
      iw = fsrc->width;
      ih = fsrc->height * fsrc->nchan;   // assumes channels are not interleaved pixel-wise

      if(!dc->isInBounds())              // is turn parameter out-of-bounds?
      { Frame::copy_data(fdst,fsrc);     // then just pass the data through - if fsrc and fdst were pointers to pointers we could avoid this copy
        return 1; //return success
      }
      DBG("In ResonantWrap::work.\r\n");

      //REMIND(fsrc->totif("ResonantWrap-src.tif"));
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
      //REMIND(fdst->totif("ResonantWrap-dst.tif"));
      return 1; // success

    }



  } // end namespace task

  namespace worker {

    ResonantWrapAgent::ResonantWrapAgent()
      : WorkAgent<TaskType,Config>("ResonantWrap")
      ,_notify_out_of_bounds_update(INVALID_HANDLE_VALUE),
      _is_in_bounds(false)
    {
      __common_setup();
    }

    ResonantWrapAgent::ResonantWrapAgent( Config *config )
      :WorkAgent<TaskType,Config>(config,"ResonantWrap")
      ,_notify_out_of_bounds_update(INVALID_HANDLE_VALUE)
      ,_is_in_bounds(false)
    {
       __common_setup();
    }

    ResonantWrapAgent::~ResonantWrapAgent()
    { Guarded_Assert_WinErr__NoPanic(CloseHandle(_notify_out_of_bounds_update));
    }

    float
    ResonantWrapAgent::turn()
    { return get_config().turn_px();
    }

    void
    ResonantWrapAgent::setTurn(float turn)
    {
      Config c = get_config();
      c.set_turn_px(turn);
      set_config(c); // FIXME only ok because the microscope is stopped when this happens
    }

    int
    ResonantWrapAgent::setTurnNoWait(float turn)
    {
      Config c = get_config();
      c.set_turn_px(turn);
      return set_config_nowait(c);
    }

    /*
    This code doesn't appear to be used anywhere.

    I added it so the worker could notify the main thread of a problem.
    Getting the range for a valid "turn" is difficult because it depends
    on the frame size which isn't simple to calculate at this point.

    I'm not sure if I need this mechanism.  If I do it might be nice to have
    around in other devices or workers, so I might generalize it.

    I'm also not sure it works like I want it to.  The wait will wake up
    on an error, but is_in_bounds might change in the meantime.  The wait
    could act as a flag on it's own?  Anyway, not sure how this is
    going to get used yet...
    */
    inline bool
    ResonantWrapAgent::isInBounds(void)
    { return _is_in_bounds;
    }

    void
    ResonantWrapAgent::setIsInBounds(bool val)
    { //only one thread writes _is_in_bounds
      //several may read
      if(val==_is_in_bounds) return;

      _is_in_bounds=val;
      SetEvent(_notify_out_of_bounds_update);
    }

    bool
    ResonantWrapAgent::waitForOOBUpdate(DWORD timeout_ms)
    { HANDLE hs[] = {this->_notify_out_of_bounds_update,
                     this->_agent->_notify_stop};
      DWORD res;
      ResetEvent(_notify_out_of_bounds_update);
      res = WaitForMultipleObjects(2,hs,FALSE,timeout_ms);
      return res == WAIT_OBJECT_0;
    }

    void ResonantWrapAgent::__common_setup()
    {
      Guarded_Assert_WinErr(
        _notify_out_of_bounds_update
        = CreateEvent(NULL, /*sec attr*/
        TRUE, /*?manual reset*/
        TRUE, /*?initial state - default oob*/
        NULL) /*name*/);
    }

  }  // namespace worker

}
