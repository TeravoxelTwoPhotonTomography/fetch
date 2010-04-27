/*
 * Terminator.cpp
 *
 *  Created on: Apr 23, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "Terminator.h"

namespace fetch
{
  namespace task
  {
    unsigned int
    Terminator::run(TerminalAgent *agent)
    {
      asynq **q;   // input queues (all input channels)
      void **buf;  // array of token buffers
      size_t *szs; // array of buffer sizes
      unsigned int i, n;

      q = agent->in->contents;
      buf = (void**)  (Guarded_Malloc(in->nelem * sizeof(void*),
                                     "Worker device task - Terminator"));
      szs = (size_t*) (Guarded_Malloc(in->nelem * sizeof(size_t),
                                     "Worker device task - Terminator"));
      n = in->nelem;
      // alloc the token buffers
      for (i = 0; i < n; i++)
      { buf[i] = Asynq_Token_Buffer_Alloc(q[i]);
        szs[i] = q[i]->q->buffer_size_bytes;
      }

      // main loop
      do
      {
        Asynq_Pop_Try(q[i % n], buf + i%n,szs[i%n]);
        szs[i%n] = q[i%n]->q->buffer_size_bytes;
        i++;
      } while (WAIT_OBJECT_0
          != WaitForSingleObject(d->notify_stop,
                                 TERMINATOR_DEFAULT_WAIT_TIME_MS));
      // cleanup
      for (i = 0; i < n; i++)
        Asynq_Token_Buffer_Free(buf[i]);

      free(buf);
      return 0; // success
    }

    void
    Terminator::alloc_output_queues(Agent *agent)
    { //NULL - No output
    }

  }
}

