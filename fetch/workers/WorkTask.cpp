/*
 * WorkTask.cpp
 *
 *  Created on: Apr 22, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "WorkTask.h"
#include "frame.h"

namespace fetch
{
  namespace task
  {

    void WorkTask::alloc_output_queues(Agent *agent)
    { // Allocates an output queue on out[0] that has matching storage to in[0].
      agent->_alloc_qs_easy(agent->out,
                            1,                                          // number of output channels to allocate
                            agent->in->contents->q->ring->count,         // copy number of output buffers from input queue
                            agent->in->contents->q->buffer_size_bytes);  // copy buffer size from input queue
    }

    void UpdateableWorkTask::alloc_output_queues(Agent *agent)
    { // Allocates an output queue on out[0] that has matching storage to in[0].
      agent->_alloc_qs_easy(agent->out,
                            1,                                          // number of output channels to allocate
                            agent->in->contents->q->ring->count,         // copy number of output buffers from input queue
                            agent->in->contents->q->buffer_size_bytes);  // copy buffer size from input queue
    }

    unsigned int
    OneToOneWorkTask::run(Agent *d)
    { unsigned int sts = 0; // success=0, fail=1
      asynq *qsrc = in->contents[0],
            *qdst = out->contents[0];
      Frame *fsrc =  (Frame*)Asynq_Token_Buffer_Alloc(qsrc),
            *fdst =  (Frame*)Asynq_Token_Buffer_Alloc(qdst);
      do
      { while( Asynq_Pop_Try(qsrc, (void**)&fsrc, fsrc->size_bytes()) )
        { fsrc->format(fdst);
          goto_if_fail(
            work(d,fdst,fsrc),
            WorkFunctionFailure);
          goto_if_fail(
            Asynq_Push_Timed( qdst, (void**)&fdst, fdst->size_bytes(), WORKER_DEFAULT_TIMEOUT ),
            OutputQueueTimeoutError);
        }
      } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );

    Finalize:
      Asynq_Token_Buffer_Free(fsrc);
      Asynq_Token_Buffer_Free(fdst);
      return sts;
    // Error handling
    WorkFunctionFailure:
      warning("Work function failed.\r\n");
      sts = 1;
      goto Finalize;
    OutputQueueTimeoutError:
      warning("Pushing to output queue timed out.\r\n");
      sts = 1;
      goto Finalize;
    }

  }
}







