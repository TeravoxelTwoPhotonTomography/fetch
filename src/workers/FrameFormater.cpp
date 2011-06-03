/*
 * FrameFormatter.cpp
 *
 *  Created on: Apr 23, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */


#include "WorkAgent.h"
#include "FrameFormater.h"
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
    unsigned int
    FrameFormatter::
    work(IDevice *idc, Frame *fdst, Frame *fsrc)
    {
      //fsrc->dump("FrameFormatter-src.%s",TypeStrFromID(fsrc->rtti));
      Frame_With_Interleaved_Planes::translate(fdst,fsrc);
      //fdst->dump("FrameFormatter-dst.%s",TypeStrFromID(fdst->rtti));
      return 1; // success
    }

    // fdst comes in formatted by src.   
    // Cast to planewise frame
    unsigned int FrameFormatter::reshape( IDevice *d, Frame *fdst )
    {
      //Frame_With_Interleaved_Planes ref(fdst->width,fdst->height,fdst->nchan,fdst->rtti);
      //ref.format(fdst);
      return 1; //success
    }

  }
}
