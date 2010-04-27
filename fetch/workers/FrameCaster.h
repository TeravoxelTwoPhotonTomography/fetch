/*
 * FrameCast.h
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
 * FrameCast
 * =========
 *
 * WorkTask that transforms the pixel type of Frames.
 *
 * Usage
 * -----
 * Scanner2D source;
 * WorkAgent<FrameCast<f32>> step1(source,0,NULL);
 */
#pragma once

#include "WorkTask.h"
#include "../util/util-image.h"
#include "../frame.h"
#include "../types.h"

namespace fetch
{

  namespace task
  {

    template<typename Tdst>
    class FrameCast : public fetch::task::OneToOneWorkTask
    {
      public:
        unsigned int work(WorkAgent *agent, Frame *fdst, Frame *fsrc);
    };

    /*
     * Implementation
     */
    template<typename Tdst>
      inline unsigned int
      FrameCast<Tdst>::work(WorkAgent *agent, Frame *fdst, Frame *fsrc)
      { void  *s,*d;
        size_t dst_pitch, src_pitch;
        fdst->rtti = TypeID<Tdst> ();
        fdst->Bpp = g_type_attributes[TypeID<Tdst> ()].bytes;
        fdst->compute_pitches(dst_pitch);
        fsrc->compute_pitches(src_pitch);
        s = fsrc->data;
        d = fdst->data;
        switch (fsrc->rtti) {
          case u8 : imCastCopy<Tdst,u8 >(d, dst_pitch, s, src_pitch); break;
          case u16: imCastCopy<Tdst,u16>(d, dst_pitch, s, src_pitch); break;
          case u32: imCastCopy<Tdst,u32>(d, dst_pitch, s, src_pitch); break;
          case u64: imCastCopy<Tdst,u64>(d, dst_pitch, s, src_pitch); break;
          case i8 : imCastCopy<Tdst,i8 >(d, dst_pitch, s, src_pitch); break;
          case i16: imCastCopy<Tdst,i16>(d, dst_pitch, s, src_pitch); break;
          case i32: imCastCopy<Tdst,i32>(d, dst_pitch, s, src_pitch); break;
          case i64: imCastCopy<Tdst,i64>(d, dst_pitch, s, src_pitch); break;
          case f32: imCastCopy<Tdst,f32>(d, dst_pitch, s, src_pitch); break;
          case f64: imCastCopy<Tdst,f64>(d, dst_pitch, s, src_pitch); break;
          default:
            warning("Could not interpret source type.\r\n");
            return 0; // failure
            break;
        }
        return 1; // success
      }

  }
}



