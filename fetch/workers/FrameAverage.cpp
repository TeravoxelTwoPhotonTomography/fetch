/*
 * FrameAverage.cpp
 *
 *  Created on: Apr 22, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Notes for change to resizable queue
 * -----------------------------------
 * - need current buffer size for push/pop...just need to keep track
 * - what happens if the source buffer changes in the middle of an average
 */
#include "stdafx.h"
#include "FrameAverage.h"

using namespace fetch::worker;

namespace fetch
{

  namespace task
  { 
    unsigned int
    FrameAverage::run(Agent *a)
    { FrameAverageAgent *agent = dynamic_cast<FrameAverageAgent*>(a);
      int every = agent->config;

      asynq *qsrc = agent->in->contents[ 0 ],
            *qdst = agent->out->contents[ 0 ];

      Frame *fsrc = (Frame*) Asynq_Token_Buffer_Alloc(qsrc),
            *fdst = (Frame*) Asynq_Token_Buffer_Alloc(qdst);
      f32 *buf, *acc = NULL;
      size_t dst_bytes = qdst->q->buffer_size_bytes;
      { unsigned int i,count=0;

        do
        {
          size_t src_bytes = qsrc->q->buffer_size_bytes,
                    nbytes = src_bytes - sizeof(Frame), //bytes in acc
                     nelem = nbytes / sizeof(f32);

          // First one
          if(!agent->is_stopping() && Asynq_Pop(qsrc,(void**)&fsrc,src_bytes))
          {
            if(fsrc->size_bytes()>dst_bytes)
              Guarded_Assert(fdst = (Frame*) realloc(fdst,dst_bytes = fsrc->size_bytes()));

            fsrc->format(fdst);
            acc = (f32*) fdst->data;
            memcpy(acc,fsrc->data,nbytes);
          } else
            continue;

          // The rest
          while(!agent->is_stopping() && Asynq_Pop(qsrc, (void**)&fsrc, fsrc->size_bytes()) )
          { buf = (f32*) fsrc->data;

            ++count;
            if( count % every == 0 )           // emit and reset every so often
            { float norm = (float)every;       //   average
              for(i=0;i<nelem;++i)
                acc[i]/=norm;

              goto_if_fail(                    //   push - wait till successful
                Asynq_Push_Timed( qdst, (void**)&fdst, fdst->size_bytes(), WORKER_DEFAULT_TIMEOUT ),
                OutputQueueTimeoutError);
              if(fsrc->size_bytes()>dst_bytes)
                  Guarded_Assert(fdst = (Frame*)realloc(fdst,dst_bytes = fsrc->size_bytes()));
              fsrc->format(fdst);              // Initialize the accumulator
              acc = (f32*)fdst->data;
              memcpy(acc,fsrc->data,nbytes);
            } else
            { for (i = 0; i < nelem; ++i)      // accumulate
                acc[i] += buf[i];
            }
          }
        } while (!agent->is_stopping());
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