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
#include "stdafx.h"
#include "Terminator.h"

#if 0
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG(...)
#endif

namespace fetch
{
  namespace task
  {
    unsigned int
    Terminator::run(IDevice *d)
    {
      asynq **q;   // input queues (all input channels)
      void **buf;  // array of token buffers
      size_t *szs; // array of buffer sizes
      unsigned int i, n;

      q = d->_in->contents;
      n = d->_in->nelem;
      buf = (void**)  (Guarded_Malloc(n * sizeof(void*),
                                     "Worker device task - Terminator"));
      szs = (size_t*) (Guarded_Malloc(n * sizeof(size_t),
                                     "Worker device task - Terminator"));
      // alloc the token buffers
      for (i = 0; i < n; i++)
      { buf[i] = Asynq_Token_Buffer_Alloc(q[i]);
        szs[i] = q[i]->q->buffer_size_bytes;
      }

      // main loop
      do
      { 
#if 0
        if( !Asynq_Is_Empty(q[0]) ) DBG("Convenient break point\r\n");
#endif
        if( Asynq_Pop(q[i % n], buf + i%n,szs[i%n]) )
        { DBG("Task: Terminator: trashing buffer on queue %d (iter: %d)\r\n",i%n,i);
          szs[i%n] = q[i%n]->q->buffer_size_bytes;
        }
        i++;
      } while (!d->_agent->is_stopping());
      
      // cleanup
      for (i = 0; i < n; i++)
        Asynq_Token_Buffer_Free(buf[i]);

      free(buf);
      return 0; // success
    }

    void
    Terminator::alloc_output_queues(IDevice *d)
    { //noop - No output
    }

  }
}

