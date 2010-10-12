/*
 * WorkTask.cpp
 *
 *  Created on: Apr 22, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "stdafx.h"
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
                        d->_in->contents[0]->q->ring->nelem,        // copy number of output buffers from input queue
                        d->_in->contents[0]->q->buffer_size_bytes); // copy buffer size from input queue
    }

  }
}