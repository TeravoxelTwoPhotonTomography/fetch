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
#include "config.h"
#include "FrameAverage.h"

using namespace fetch::worker;

namespace fetch
{

  namespace task
  { 
    unsigned int
    FrameAverage::run(IDevice *idc)
    { FrameAverageAgent *dc = dynamic_cast<FrameAverageAgent*>(idc);
      int every = dc->get_config().ntimes(); //agent->config;
      int eflag = 0;

      Chan *qsrc = dc->_in->contents[ 0 ],
           *qdst = dc->_out->contents[ 0 ],
           *reader, *writer;

      Frame *fsrc = (Frame*) Chan_Token_Buffer_Alloc(qsrc),
            *fdst = (Frame*) Chan_Token_Buffer_Alloc(qdst);
      f32 *buf, *acc = NULL;
      size_t dst_bytes = Chan_Buffer_Size_Bytes(qdst);
      
      reader = Chan_Open(qsrc,CHAN_READ);
      writer = Chan_Open(qdst,CHAN_WRITE);
      if(every<=1)
      // Do nothing, just pass through
      { while(CHAN_SUCCESS( Chan_Next(reader,(void**)&fsrc,Chan_Buffer_Size_Bytes(qsrc)) )) // !dc->_agent->is_stopping() &&
          goto_if_fail(  //   push - wait till successful
                CHAN_SUCCESS(Chan_Next(writer,(void**)&fsrc, fsrc->size_bytes() )),
                OutputQueueTimeoutError);
      
      } else            
      { unsigned int i,count=0;

        do
        {
          size_t src_bytes = Chan_Buffer_Size_Bytes(qsrc),
                    nbytes = src_bytes - sizeof(Frame), //bytes in acc
                     nelem = nbytes / sizeof(f32);

          // First one
          if(CHAN_SUCCESS(Chan_Next(reader,(void**)&fsrc,src_bytes) ))     // !dc->_agent->is_stopping() &&
          {

            src_bytes = fsrc->size_bytes();
            nbytes    = src_bytes - sizeof(Frame); //bytes in acc
            nelem     = nbytes / sizeof(f32);

            if(fsrc->size_bytes()>dst_bytes)              
              Guarded_Assert(fdst = (Frame*) realloc(fdst,dst_bytes = fsrc->size_bytes()));

            fsrc->format(fdst);
            acc = (f32*) fdst->data;
            memcpy(acc,fsrc->data,src_bytes);
          } else
            continue;

          // The rest
          while(CHAN_SUCCESS( Chan_Next(reader, (void**)&fsrc, fsrc->size_bytes()) ))    //!dc->_agent->is_stopping() && 
          { buf = (f32*) fsrc->data;

            src_bytes = fsrc->size_bytes();
            nbytes    = src_bytes - sizeof(Frame); //bytes in acc
            nelem     = nbytes / sizeof(f32);

            //fsrc->dump("FrameAveragerIn_%03d.%s",count,TypeStrFromID(fdst->rtti));
            //fdst->dump("FrameAveragerOut_%03d.%s",count,TypeStrFromID(fdst->rtti));

            ++count;
            if( count % every == 0 )           // emit and reset every so often
            { float norm = (float)every;       //   average
              for(i=0;i<nelem;++i)
                acc[i]/=norm;

              goto_if_fail(                    //   push - wait till successful
                CHAN_SUCCESS( Chan_Next(writer,(void**)&fdst, fdst->size_bytes()) ),
                OutputQueueTimeoutError);
              if(fsrc->size_bytes()>dst_bytes)
                  Guarded_Assert(fdst = (Frame*)realloc(fdst,dst_bytes = fsrc->size_bytes()));
              fsrc->format(fdst);              // Initialize the accumulator
              acc = (f32*)fdst->data;
              memcpy(acc,fsrc->data,dst_bytes);
            } else
            { for (i = 0; i < nelem; ++i)      // accumulate
                acc[i] += buf[i];
            }
          }
        } while (!dc->_agent->is_stopping());
      }
Finalize:
      Chan_Close(reader);
      Chan_Close(writer);
      Chan_Token_Buffer_Free(fsrc);
      Chan_Token_Buffer_Free(fdst);
      return eflag;
OutputQueueTimeoutError:
      warning("Pushing to output queue timeout\r\n.");
      eflag=1;
      goto Finalize;
    }

  }
}