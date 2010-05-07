/*
 * PixelWiseAverager.cpp
 *
 *  Created on: Apr 23, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "stdafx.h"
#include "WorkAgent.h"
#include "PixelWiseAverager.h"

using namespace fetch::worker;

namespace fetch
{
  namespace task
  {
    template<typename Tsrc>
    void pwa(void *dst, void *src, int ntimes, int nelem)
    { Tsrc *src_cur,
           *buf = (Tsrc*)src;
      f32  *acc_cur,
           *acc = (f32*)dst,
            norm = (f32)ntimes;
      memset(acc, 0, nelem*sizeof(f32));         // initialize accumulator
      acc_cur = acc;                             // accumulate and average
      for (src_cur = buf; src_cur < buf + nelem;)
      { for (int i = 0; i < ntimes; ++i)
          *acc_cur += *src_cur++;
        *acc_cur /= norm;
        ++acc_cur;
      }
    }
    
    unsigned int
    PixelWiseAverager::
    work(Agent *pwaa, Frame *fdst, Frame *fsrc)
    { PixelWiseAveragerAgent *agent = dynamic_cast<PixelWiseAveragerAgent*>(pwaa);
      int N;
      size_t nbytes = fsrc->size_bytes(),
             nelem  = (nbytes - sizeof(Frame))/fsrc->Bpp;
      //fsrc->dump("pixel-averager-source.f32");
      
      N = agent->config;      
      fdst->width /= N;      // Adjust output format
      fdst->rtti   = id_f32;
      fdst->Bpp    = 4;
      debug("In PixelWiseAverager::work.\r\n");      
      fsrc->dump("PixelWiseAverager-src.%s",TypeStrFromID(fsrc->rtti));
      switch(fsrc->rtti)
      { 
        case id_u8 : pwa<u8 >(fdst->data,fsrc->data,N,nelem); break;
        case id_u16: pwa<u16>(fdst->data,fsrc->data,N,nelem); break;
        case id_u32: pwa<u32>(fdst->data,fsrc->data,N,nelem); break;
        case id_u64: pwa<u64>(fdst->data,fsrc->data,N,nelem); break;
        case id_i8 : pwa<i8 >(fdst->data,fsrc->data,N,nelem); break;
        case id_i16: pwa<i16>(fdst->data,fsrc->data,N,nelem); break;
        case id_i32: pwa<i32>(fdst->data,fsrc->data,N,nelem); break;
        case id_i64: pwa<i64>(fdst->data,fsrc->data,N,nelem); break;
        case id_f32: pwa<f32>(fdst->data,fsrc->data,N,nelem); break;
        //case id_f64: pwa<f64>(fdst->data,fsrc->data,N,nelem); break;
        default:
          error("Unrecognized source type (id=%d).\r\n",fsrc->rtti);        
      }
      fdst->dump("PixelWiseAverager-dst.%s",TypeStrFromID(fdst->rtti));
      return 1; // success
    }

  }
}