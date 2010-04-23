/*
 * PixelWiseAverager.cpp
 *
 *  Created on: Apr 23, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "WorkAgent.h"
#include "PixelWiseAverager.h"

namespace fetch
{
  namespace task
  {
    unsigned int
    PixelWiseAverager::
    work(PixelWiseAveragerAgent *agent, Frame *dst, Frame *src)
    { f32 *buf,*acc,*src_cur,*acc_cur;
      int N;
      f32 norm;

      N = agent->config;
      norm = (float)N;

      fdst->width /= N;                           // Adjust output width
      acc = (f32*) fdst->data;
      buf = (f32*) fsrc->data;
      memset(acc, 0, nbytes);                     // initialize accumulator

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

