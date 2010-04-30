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
    unsigned int
    PixelWiseAverager::
    work(Agent *pwaa, Frame *fdst, Frame *fsrc)
    { PixelWiseAveragerAgent *agent = dynamic_cast<PixelWiseAveragerAgent*>(pwaa);
      f32 *buf,*acc,*src_cur,*acc_cur;
      int N;
      f32 norm;
      size_t nbytes = fsrc->size_bytes(),
             nelem  = (nbytes - sizeof(Frame))/fsrc->Bpp;

      N = agent->config;
      norm = (float)N;

      fdst->width /= N;                           // Adjust output width
      acc = (f32*) fdst->data;
      buf = (f32*) fsrc->data;
      memset(acc, 0, nbytes);         // initialize accumulator

      //fsrc->dump("pixel-averager-source.f32");

      acc_cur = acc;                              // accumulate and average
      for (src_cur = buf; src_cur < buf + nelem;)
      { for (int i = 0; i < N; ++i)
          *acc_cur += *src_cur++;
        *acc_cur /= norm;
        ++acc_cur;
      }

      return 1; // success
    }

  }
}