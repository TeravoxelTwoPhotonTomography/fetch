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
#include <image.h>
}

using namespace fetch::worker;

#if 0
#define PROFILE(expr,msg) \
      do { TicTocTimer t = tic(); \
        expr; \
        debug("%s(%d):"ENDL"\tProfiling %s"ENDL"\t%s takes %f ms.",__FILE__,__LINE__,#expr,msg,1000.0*toc(&t)); \
      } while(0) 
#else
#define PROFILE(expr,msg) expr
#endif

#define ASRT(expr) \
  if(!(expr)) \
  { error("%s(%d)" ENDL "\tExpression evaluated as False." ENDL "\t%s" ENDL, __FILE__,__LINE__,#expr); \
  }

#define CHK(expr,lbl) \
  if(!(expr)) \
  { warning("%s(%d)" ENDL "\tExpression evaluated as False." ENDL "\t%s" ENDL, __FILE__,__LINE__,#expr); \
    goto lbl; \
  }
// I use REMIND to warn me in case I leave in a debugging expression.
#define REMIND(expr) \
  warning("%s(%d)" ENDL "\tEvaluating expression." ENDL "\t%s" ENDL, __FILE__,__LINE__,#expr); \
  (expr)

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


    fdst->width  = dims[0];
    fdst->height = dims[1];
    fdst->nchan  = dims[2];
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
      
      //REMIND( mylib::Write_Image("ResonantUnwarp_in.tif",&in,mylib::DONT_PRESS) );
//#ifdef HAVE_CUDA 
#if 0
      PROFILE(CHK( unwarp_gpu(&out,&in,duty),Error),"Unwarp GPU");
#else
      PROFILE(CHK( unwarp_cpu(&out,&in,duty),Error),"Unwarp CPU");      
#endif
      //REMIND( mylib::Write_Image("ResonantUnwarp_out.tif",&out,mylib::DONT_PRESS) );

      return 1; // success
Error:
      return 0; // failed
    }

}} // end namespace fetch::task

////////////////////////////////////////////////////////////////////////////////
//  ResonantUnwarpAgent   //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace fetch {

  bool operator==(const cfg::worker::ResonantUnwarp& a, const cfg::worker::ResonantUnwarp& b)
  { return fabs(a.duty()-b.duty())<1e-3;
  }
  bool operator!=(const cfg::worker::ResonantUnwarp& a, const cfg::worker::ResonantUnwarp& b)
  { return !(a==b);
  }

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
    { 
/* Don't do:                      Don't use this block unless you _know_ the microscope is stopped()
      Config c = get_config();    keeping it here as a warning
      c.set_duty(v);
      set_config(c);
*/
      _config->set_duty(v);       // FIXME: if I were actually changing this on the fly, risk disagreement between task's reshape() and work()
    }

    int
    ResonantUnwarpAgent::
      setDutyNoWait(float v)
    {
/* Don't do:
      Config c = get_config();
      c.set_duty(v);
      return set_config_nowait(c);
*/
      _config->set_duty(v);
      return 1; /*success*/
    }

}} // end namespace fetch::worker
