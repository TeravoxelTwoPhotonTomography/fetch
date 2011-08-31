/*
 * HorizontalDownsampler.cpp
 *
 *  Created on: Apr 23, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */


#include "WorkAgent.h"
#include "HorizontalDownsampler.h"
#include "util\util-image.h"

#if 0
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG
#endif

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
      memset(acc, 0, nelem*sizeof(f32)/ntimes);  // initialize accumulator
      acc_cur = acc;                             // accumulate and average
      for (src_cur = buf; src_cur < buf + nelem;)
      { for (int i = 0; i < ntimes; ++i)
          *acc_cur += *src_cur++;
        *acc_cur /= norm;
        ++acc_cur;
      }
    }
    
    unsigned int
    HorizontalDownsampler::
    work(IDevice *idc, Frame *fdst, Frame *fsrc)
    { HorizontalDownsampleAgent *dc = dynamic_cast<HorizontalDownsampleAgent*>(idc);
      int N;
      size_t 
        nbytes = fsrc->size_bytes(),
        nelem  = (nbytes - sizeof(Frame))/fsrc->Bpp;
        
      N = dc->get_config().ntimes();      
      if(N==1)
      {
        size_t sp[4],dp[4],ds[3],ss[3];
        fdst->compute_pitches(dp);
        fsrc->compute_pitches(sp);
        fdst->get_shape(ds);
        fsrc->get_shape(ss);
        //fsrc->dump("HorizontalDownsampler-src.%s",TypeStrFromID(fsrc->rtti));
        switch(fsrc->rtti)
        {
        case id_u8 : imCopy<f32,u8> ((f32*)fdst->data,dp,(u8* )fsrc->data,sp,ss); break;
        case id_u16: imCopy<f32,u16>((f32*)fdst->data,dp,(u16*)fsrc->data,sp,ss); break;
        case id_u32: imCopy<f32,u32>((f32*)fdst->data,dp,(u32*)fsrc->data,sp,ss); break;
        case id_u64: imCopy<f32,u64>((f32*)fdst->data,dp,(u64*)fsrc->data,sp,ss); break;
        case id_i8 : imCopy<f32,i8> ((f32*)fdst->data,dp,(i8* )fsrc->data,sp,ss); break;
        case id_i16: imCopy<f32,i16>((f32*)fdst->data,dp,(i16*)fsrc->data,sp,ss); break;
        case id_i32: imCopy<f32,i32>((f32*)fdst->data,dp,(i32*)fsrc->data,sp,ss); break;
        case id_i64: imCopy<f32,i64>((f32*)fdst->data,dp,(i64*)fsrc->data,sp,ss); break;
        case id_f32: imCopy<f32,f32>((f32*)fdst->data,dp,(f32*)fsrc->data,sp,ss); break;
          //case id_f64: pwa<f64>(fdst->data,fsrc->data,N,nelem); break;
        default:
          error("Unrecognized source type (id=%d).\r\n",fsrc->rtti);        
        }
        //fdst->dump("HorizontalDownsampler-dst.%s",TypeStrFromID(fdst->rtti));
        return 1; //success
      }

      DBG("In HorizontalDownsampler::work.\r\n");
      //fsrc->dump("HorizontalDownsampler-src.%s",TypeStrFromID(fsrc->rtti));
      switch(fsrc->rtti)
      { 
        case id_u8 : pwa<u8 >(fdst->data,fsrc->data,N,(size_t)nelem); break;
        case id_u16: pwa<u16>(fdst->data,fsrc->data,N,(size_t)nelem); break;
        case id_u32: pwa<u32>(fdst->data,fsrc->data,N,(size_t)nelem); break;
        case id_u64: pwa<u64>(fdst->data,fsrc->data,N,(size_t)nelem); break;
        case id_i8 : pwa<i8 >(fdst->data,fsrc->data,N,(size_t)nelem); break;
        case id_i16: pwa<i16>(fdst->data,fsrc->data,N,(size_t)nelem); break;
        case id_i32: pwa<i32>(fdst->data,fsrc->data,N,(size_t)nelem); break;
        case id_i64: pwa<i64>(fdst->data,fsrc->data,N,(size_t)nelem); break;
        case id_f32: pwa<f32>(fdst->data,fsrc->data,N,(size_t)nelem); break;
        //case id_f64: pwa<f64>(fdst->data,fsrc->data,N,nelem); break;
        default:
          error("Unrecognized source type (id=%d).\r\n",fsrc->rtti);        
      }
      //fdst->dump("HorizontalDownsampler-dst.%s",TypeStrFromID(fdst->rtti));
      return 1; // success
    }

    unsigned int HorizontalDownsampler::reshape( IDevice *d, Frame *fdst )
    {
      HorizontalDownsampleAgent *dc = dynamic_cast<HorizontalDownsampleAgent*>(d);
      int N;
      size_t 
        nbytes = fdst->size_bytes(),
        nelem  = (nbytes - sizeof(Frame))/fdst->Bpp;

      N = dc->get_config().ntimes();
      fdst->width /= N;      // Adjust output format
      fdst->rtti   = id_f32;
      fdst->Bpp    = 4;
      return 1; //success
    }

  }
}