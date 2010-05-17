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
      get_dim(iw,ih,turn,&ow,&oh);
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

  }

}
