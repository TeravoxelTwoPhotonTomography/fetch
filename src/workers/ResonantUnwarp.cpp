/*
 * ResonantUnwarp.cpp
 *
 *  Created on: Sep 12, 2011
 *      Author: Nathan
 */
/*
 * Copyright 2010,2011 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#include "ResonantUnwarp.h"
#include "algo/unwarp.h"
#include "util/util-mylib.h"
#include "common.h"
#include "config.h"
namespace mylib {
#include <array.h>
}

using namespace fetch::worker;

#define ASRT(expr) \
  if(!(expr)) \
  { error("%s(%d)" ENDL "\tExpression evaluated as False." ENDL "\t%s" ENDL, __FILE__,__LINE__,#expr); \
  }

#define CHK(expr,lbl) \
  if(!(expr)) \
  { warning("%s(%d)" ENDL "\tExpression evaluated as False." ENDL "\t%s" ENDL, __FILE__,__LINE__,#expr); \
    goto lbl; \
  }

////////////////////////////////////////////////////////////////////////////////
//  ResonantUnwarp Task   //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace fetch {
namespace task {

  unsigned int 
    ResonantUnwarp::
    reshape(IDevice *d, Frame *fdst)
  {
    ResonantUnwarpAgent *dc;
    Dimn_Type dims[3];
    float duty;
    ASRT(dc = dynamic_cast<ResonantUnwarpAgent*>(d));
    duty = dc->_config->duty();

    dims[0] = fdst->width;
    dims[1] = fdst->height;
    dims[2] = fdst->nchan;

    unwarp_get_dims_ip(dims,duty);
    return 1; // success
  }

  unsigned int
    ResonantUnwarp::
    work(IDevice *idc, Frame *fdst, Frame *fsrc)
    { ResonantUnwarpAgent *dc;
      float duty;
      mylib::Array in,out;
      Dimn_Type idims[3],odims[3];

      ASRT(dc = dynamic_cast<ResonantUnwarpAgent*>(idc));
      duty = dc->_config->duty();

      mylib::castFetchFrameToDummyArray(&in,fsrc,idims);
      mylib::castFetchFrameToDummyArray(&out,fdst,odims);

#ifdef HAVE_CUDA
      CHK( unwarp_gpu(&out,&in,duty),Error);
#else
      CHK( unwarp_cpu(&out,&in,duty),Error);
#endif
      return 1; // success
Error:
      return 0; // failed
    }

}} // end namespace fetch::task

////////////////////////////////////////////////////////////////////////////////
//  ResonantUnwarpAgent   //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace fetch {
namespace worker{

    ResonantUnwarpAgent::
      ResonantUnwarpAgent()
      : WorkAgent<TaskType,Config>("ResonantUnwarp")
    { 
    }

    ResonantUnwarpAgent::
      ResonantUnwarpAgent( Config *config )
      :WorkAgent<TaskType,Config>(config,"ResonantUnwarp")
    { 
    }

    float
    ResonantUnwarpAgent::
      duty()
    { return get_config().duty();
    }


    void
    ResonantUnwarpAgent::
      setDuty(float v)
    { Config c = get_config();
      c.set_duty(v);
      set_config(c);
    }

    int
    ResonantUnwarpAgent::
      setDutyNoWait(float v)
    { Config c = get_config();
      c.set_duty(v);
      return set_config_nowait(c);
    }

}} // end namespace fetch::worker