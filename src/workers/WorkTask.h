/*
 * WorkTask.h
 *
 *  Created on: Apr 22, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/*
 * WorkTask
 * UpdateableWorkTask
 *
 *      WorkTasks are simplified tasks that are used as template parameters
 *      to WorkAgents.  Together WorkAgents and WorkTasks are used to define
 *      transformation pipelines for messages.
 *
 *      WorkAgents and WorkTasks don't require the full state management
 *      afforded by the Agent/Task model.  They play a specialized role.
 *
 *      Usually work tasks don't require any initialization of resources.
 *      Exceptions to this rule would be, for example, tasks that need to
 *      add static data to a buffer that will be used during the run.
 *
 *      <alloc_output_queues(Agent *agent)>
 *
 *              This is responsible for allocating output queues.  It is called
 *              during construction of the associated WorkAgent<>.  The
 *              default implementation assumes a single output queue at out[0]
 *              that is the same size as the in[0] queue.
 *
 * UpdateableWorkTask
 *
 *      These are used with WorkAgents where the parameters could be adjusted
 *      on the fly by the application.  They should be used with appropriate
 *      WorkTask children.
 *
 * OneToOneWorkTask
 *
 *      These are tasks who define a function, work, that maps a message to a
 *      message.  That is, for every element,e, popped from the source queue, a
 *      single result, work(e), is pushed onto the result queue.
 *
 *      This is a pretty common form, so the run() loop is already written.
 *      Subclasses just need to define the work() function.  The reshape()
 *      function should be reimplemented if the destination frame's shape
 *      is different than the source's.
 *
 *      See FrameCaster.h for an example.
 */
#pragma once

#include "WorkAgent.h"
#include "task.h"
#include "frame.h"
#include "util/timestream.h"

#define PROFILE
#ifdef PROFILE // PROFILING
#define TS_OPEN(...)    timestream_t ts__=timestream_open(__VA_ARGS__)
#define TS_TIC          timestream_tic(ts__)
#define TS_TOC          timestream_toc(ts__)
#define TS_CLOSE        timestream_close(ts__)
#else
#define TS_OPEN(...)
#define TS_TIC
#define TS_TOC
#define TS_CLOSE
#endif

#if 0
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define TRY(expr,lbl) \
  if(!(expr)) \
    { warning("%s(%d): %s"ENDL"\t%s"ENDL"\tExpression evaluated to false.",__FILE__,__LINE__,#lbl,#expr); \
      goto lbl; \
    }

namespace fetch
{

  namespace task
  {

    class WorkTask : public fetch::Task
    {
    public:
      unsigned int config(IDevice *d) {return 1;} // success.  Most WorkTasks don't need a config function.
      virtual void alloc_output_queues(IDevice *d);
    };


    template<typename TMessage>
    class OneToOneWorkTask : public WorkTask
    { public:
                unsigned int run(IDevice *d);
        virtual unsigned int work(IDevice *d, TMessage *dst, TMessage *src) = 0;
        virtual unsigned int reshape(IDevice *d, TMessage *dst) {return 1;}
    };

    template<typename TMessage>
    class InPlaceWorkTask : public WorkTask
    { public:
                unsigned int run(IDevice *d);
        virtual unsigned int work(IDevice *agent, TMessage *src) = 0;
    };


//
// Implementation
//


    template<typename TMessage>
    unsigned int
    OneToOneWorkTask<TMessage>::run(IDevice *d)
    { unsigned int sts = 0; // success=0, fail=1
      Chan *reader,*writer,
           *qsrc = d->_in->contents[0],
           *qdst = d->_out->contents[0];
      TMessage *fsrc =  (TMessage*)Chan_Token_Buffer_Alloc(qsrc),
               *fdst =  (TMessage*)Chan_Token_Buffer_Alloc(qdst);
      size_t nbytes_in  = Chan_Buffer_Size_Bytes(qsrc),
             nbytes_out = Chan_Buffer_Size_Bytes(qdst);
      TS_OPEN("timer-%s.f32",d->_agent->name());

      reader = Chan_Open(qsrc,CHAN_READ);
      writer = Chan_Open(qdst,CHAN_WRITE);
      while(CHAN_SUCCESS( Chan_Next(reader, (void**)&fsrc, nbytes_in) )) // !d->_agent->is_stopping() &&
        { DBG("%s(%d)"ENDL "\t%s just recieved"ENDL,__FILE__,__LINE__,d->_agent->name());
          nbytes_in = fsrc->size_bytes();
          fsrc->format(fdst);
          TRY(reshape(d,fdst), FormatFunctionFailure);
          nbytes_out = fdst->size_bytes();
          if(nbytes_out>Chan_Buffer_Size_Bytes(qdst))
          { Chan_Resize(writer,nbytes_out);
            TRY(fdst = (TMessage *) realloc(fdst,nbytes_out),MemoryError);
            fsrc->format(fdst);
            TRY(reshape(d,fdst),FormatFunctionFailure);
          }
          TS_TIC;
          TRY(work(d,fdst,fsrc),WorkFunctionFailure);
          TS_TOC;
          TRY(CHAN_SUCCESS(Chan_Next(writer,(void**)&fdst, nbytes_out)),OutputQueueTimeoutError);
        }

Finalize:
      TS_CLOSE;
      Chan_Close(reader);
      Chan_Close(writer);
      Chan_Token_Buffer_Free(fsrc);
      Chan_Token_Buffer_Free(fdst);
      return sts;
    // Error handling
MemoryError:
      warning("[%s] Could not reallocate destination buffer.\r\n",d->_agent->_name);
      sts = 1;
      goto Finalize;
FormatFunctionFailure:
      warning("[%s] Work function failed.\r\n",d->_agent->_name);
      sts = 1;
      goto Finalize;
WorkFunctionFailure:
      warning("[%s] Work function failed.\r\n",d->_agent->_name);
      sts = 1;
      goto Finalize;
OutputQueueTimeoutError:
      warning("[%s] Pushing to output queue timed out.\r\n",d->_agent->_name);
      sts = 1;
      goto Finalize;
    }


    template<typename TMessage>
    unsigned int
    InPlaceWorkTask<TMessage>::run(IDevice *d)
    { unsigned int sts = 0; // success=0, fail=1
      Chan
        *reader,
        *writer,
        *qsrc = d->_in->contents[0],
        *qdst = d->_out->contents[0];
      TMessage *fsrc =  (TMessage*)Chan_Token_Buffer_Alloc(qsrc);
      size_t nbytes_in  = Chan_Buffer_Size_Bytes(qsrc);
      TS_OPEN("timer-%s.f32",d->_agent->name());

      reader = Chan_Open(qsrc,CHAN_READ);
      writer = Chan_Open(qdst,CHAN_WRITE);
      while(CHAN_SUCCESS( Chan_Next(reader,(void**)&fsrc,nbytes_in) )) //!d->_agent->is_stopping() &&
        {
          DBG("%s(%d)"ENDL "\t%s just recieved"ENDL,__FILE__,__LINE__,d->_agent->name());
          nbytes_in = fsrc->size_bytes();
          TS_TIC;
          TRY(work(d,fsrc),WorkFunctionFailure);
          TS_TOC;
          nbytes_in = MAX( Chan_Buffer_Size_Bytes(qdst), nbytes_in ); // XXX - awkward
          TRY(CHAN_SUCCESS(Chan_Next(writer,(void**)&fsrc, nbytes_in)),OutputQueueTimeoutError);
        }
    Finalize:
      TS_CLOSE;
      Chan_Close(reader);
      Chan_Close(writer);
      Chan_Token_Buffer_Free(fsrc);
      return sts;
    // Error handling
    WorkFunctionFailure:
      warning("%s(%d)"ENDL "\tWork function failed."ENDL,__FILE__,__LINE__);
      sts = 1;
      goto Finalize;
    OutputQueueTimeoutError:
      warning("%s(%d)"ENDL "\tPushing to output queue timed out."ENDL,__FILE__,__LINE__);
      sts = 1;
      goto Finalize;
    }

  }
}
#undef DBG
#undef TRY

#undef TS_OPEN
#undef TS_TIC
#undef TS_TOC
#undef TS_CLOSE
