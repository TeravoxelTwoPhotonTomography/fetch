/*
 * WorkTask.cpp
 *
 *  Created on: Apr 22, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */


#include "WorkTask.h"
#include "../frame.h"

namespace fetch
{
  namespace task
  {

    void WorkTask::alloc_output_queues(IDevice *d)
    { // Allocates an output queue on out[0] that has matching storage to in[0].
      d->_alloc_qs_easy(&d->_out,
                        1,                                             // number of output channels to allocate
                        Chan_Buffer_Count(d->_in->contents[0]),        // copy number of output buffers from input queue
                        Chan_Buffer_Size_Bytes(d->_in->contents[0]));  // copy buffer size from input queue
    }

  }
}