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
    unsigned int
    OneToOneWorkTask::run(Agent *d)
    { unsigned int sts = 0; // success=0, fail=1
      asynq *qsrc = in->contents[0],
            *qdst = out->contents[0];
      Frame *fsrc =  (Frame*)Asynq_Token_Buffer_Alloc(qsrc),
            *fdst =  (Frame*)Asynq_Token_Buffer_Alloc(qdst);
      do
      { while( Asynq_Pop_Try(qsrc, (void**)&fsrc) )
        { fsrc->format(fdst);
          goto_if_fail(
            work(fdst,fsrc),
            WorkFunctionFailure);
          goto_if_fail(
            Asynq_Push_Timed( qdst, (void**)&fdst, WORKER_DEFAULT_TIMEOUT ),
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



