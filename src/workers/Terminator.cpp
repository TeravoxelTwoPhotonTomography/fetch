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
      Chan **q;   // input queues (all input channels)
      void **buf;  // array of token buffers
      size_t *szs; // array of buffer sizes
      unsigned int i, n, any;
      
      n = d->_in->nelem;
      q   = (Chan **) (Guarded_Malloc(n*sizeof(Chan*),
                                     "Worker device task - Terminator"));
      buf = (void**)  (Guarded_Malloc(n * sizeof(void*),
                                     "Worker device task - Terminator"));
      szs = (size_t*) (Guarded_Malloc(n * sizeof(size_t),
                                     "Worker device task - Terminator"));
      // alloc the token buffers
      for (i = 0; i < n; i++)
      { q[i]   = Chan_Open(d->_in->contents[i],CHAN_READ);
        buf[i] = Chan_Token_Buffer_Alloc(q[i]);
        szs[i] = Chan_Buffer_Size_Bytes(q[i]);
      }

      // main loop      
      
      do
      { 
#if 0
        if( !Chan_Is_Empty(q[0]) ) DBG("Convenient break point\r\n");
#endif
        any=0;
        for(i=0;i<n;++i)
        { any |= CHAN_SUCCESS(Chan_Next(q[i],buf+i,szs[i]));
          szs[i] = Chan_Buffer_Size_Bytes(q[i]);
        }        
      } while(any); // quits when all inputs fail to pop
      
      // cleanup
      for (i = 0; i < n; i++)
      { Chan_Token_Buffer_Free(buf[i]);
        Chan_Close(q[i]);
      }

      free(buf);
      return 0; // success
    }

    void
    Terminator::alloc_output_queues(IDevice *d)
    { //noop - No output
    }

  }
}

