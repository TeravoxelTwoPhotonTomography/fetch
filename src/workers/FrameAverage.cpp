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

#if 0
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define REMIND(expr) \
  warning("%s(%d): %s"ENDL "\tDumping debug data."ENDL, __FILE__,__LINE__,__FUNCTION__);\
  (expr)

using namespace fetch::worker;

namespace fetch
{
  bool operator==(const cfg::worker::FrameAverage& a, const cfg::worker::FrameAverage& b)
  { return a.ntimes()==b.ntimes();
  }
  bool operator!=(const cfg::worker::FrameAverage& a, const cfg::worker::FrameAverage& b)
  { return !(a==b);
  }
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
        size_t src_bytes = Chan_Buffer_Size_Bytes(qsrc),
                  nbytes = src_bytes - sizeof(Frame), //bytes in acc
                   nelem = nbytes / sizeof(f32);
        
        // - If the channel closes before the requested number of averages is acquired,
        //   the accumulator will not be emitted.
        while(CHAN_SUCCESS( Chan_Next(reader, (void**)&fsrc, src_bytes) ))    //!dc->_agent->is_stopping() && 
        { src_bytes=fsrc->size_bytes();
          DBG("%s(%d)"ENDL "\t%s"ENDL "\tRecv (count: %d[%d])"ENDL,__FILE__,__LINE__,dc->_agent->name(),count,count%every);
          buf = (f32*) fsrc->data;

          src_bytes = fsrc->size_bytes();
          nbytes    = src_bytes - sizeof(Frame); //bytes in acc
          nelem     = nbytes / sizeof(f32);
          
          //fsrc->totif("FrameAveragerIn_%03d_%s.tif",count%every,TypeStrFromID(fsrc->rtti));
          
          if( count % every == 0 )           // emit and reset every so often
          { 
            if(count>0)
            { for(i=0;i<nelem;++i)           // normalize
                acc[i]/=(float)every;
              goto_if_fail(                  // push
                CHAN_SUCCESS( Chan_Next(writer,(void**)&fdst, fdst->size_bytes()) ),
                OutputQueueTimeoutError);
            }
            if(fsrc->size_bytes()>dst_bytes)
                Guarded_Assert(fdst = (Frame*)realloc(fdst,dst_bytes = fsrc->size_bytes()));
            fsrc->format(fdst);              
            acc = (f32*)fdst->data;
            memcpy(acc,fsrc->data,dst_bytes);// Initialize the accumulator              
          } else
          { for (i = 0; i < nelem; ++i)      // accumulate
              acc[i] += buf[i];
          }
          //fdst->totif("FrameAveragerOut_%03d_%s.tif",count%every,TypeStrFromID(fdst->rtti));
          ++count;
        } // end while          
        // Finally, maybe output accumulator
        if( count>0 && (count%every==0) )            
        { for(i=0;i<nelem;++i)
            acc[i]/=(float)every;              // normalize
          goto_if_fail(                        // push
            CHAN_SUCCESS( Chan_Next(writer,(void**)&fdst, fdst->size_bytes()) ),
            OutputQueueTimeoutError);
        }
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