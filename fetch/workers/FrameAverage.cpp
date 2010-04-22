/*
 * FrameAverage.cpp
 *
 *  Created on: Apr 22, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "FrameAverage.h"

namespace fetch
{

  namespace task
  {

    unsigned int
    FrameAverage::run(FrameAverageAgent *agent)
    { int every = agent->config;

      asynq *qsrc =  in->contents[ 0 ],
            *qdst = out->contents[ 0 ];

      Frame *fsrc = (Frame*) Asynq_Token_Buffer_Alloc(qsrc),
            *fdst = (Frame*) Asynq_Token_Buffer_Alloc(qdst);
      f32 *buf, *acc = NULL;
      f32 *src_cur,*acc_cur;

      { size_t nbytes = in->contents[ 0 ]->q->buffer_size_bytes - sizeof(Frame), //bytes in acc
                nelem = nbytes / sizeof(f32);

        int i,count=0;


        do
        { // First one
          if( Asynq_Pop_Try(qsrc,(void**)&fsrc))
          { fsrc->format(fdst);
            acc = (f32*) fdst->data;
            memcpy(acc,fsrc->data,nbtyes);
          } else
            continue;

          // The rest
          while( Asynq_Pop_Try(qsrc, (void**)&fsrc) )
          { buf = (f32*) fsrc->data;

            ++count;
            if( count % every == 0 )           // emit and reset every so often
            { float norm = (float)every;       //   average
              for(i=0;i<nelem;++i)
                acc[i]/=norm;

              goto_if_fail(                    //   push - wait till successful
                Asynq_Push_Timed( qdst, (void**)&fdst, WORKER_DEFAULT_TIMEOUT ),
                OutputQueueTimeoutError);
              fsrc->format(fdst);              // Initialize the accumulator
              acc = (f32*)fdst->data;
              memcpy(acc,fsrc->data,nbytes);
            } else
            { for (i = 0; i < nelem; ++i)      // accumulate
                acc[i] += buf[i];
            }
          }
        } while ( WAIT_OBJECT_0 != WaitForSingleObject(d->notify_stop, 0) );
      }
      Asynq_Token_Buffer_Free(fsrc);
      Asynq_Token_Buffer_Free(fdst);
      return 0;
    OutputQueueTimeoutError:
      warning("Pushing to output queue timeout\r\n.");
      Asynq_Token_Buffer_Free(fsrc);
      Asynq_Token_Buffer_Free(fdst);
      return 1; // failure
    }

  }
}


