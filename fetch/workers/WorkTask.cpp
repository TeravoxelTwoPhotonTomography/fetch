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

    void WorkTask::alloc_output_queues(Agent *agent)
    { // Allocates an output queue on out[0] that has matching storage to in[0].
      agent->_alloc_qs_easy(&agent->out,
                            1,                                             // number of output channels to allocate
                            agent->in->contents[0]->q->ring->count,        // copy number of output buffers from input queue
                            agent->in->contents[0]->q->buffer_size_bytes); // copy buffer size from input queue
    }

    //void UpdateableWorkTask::alloc_output_queues(Agent *agent)
    //{ // Allocates an output queue on out[0] that has matching storage to in[0].
    //  agent->_alloc_qs_easy(agent->out,
    //                        1,                                          // number of output channels to allocate
    //                        agent->in->contents->q->ring->count,         // copy number of output buffers from input queue
    //                        agent->in->contents->q->buffer_size_bytes);  // copy buffer size from input queue
    //}



  }
}