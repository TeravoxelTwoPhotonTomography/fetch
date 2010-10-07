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
 *      Subclasses just need to define the work() function.
 *
 *      See FrameCaster.h for an example.
 */
#pragma once

#include "WorkAgent.h"
#include "../task.h"
#include "../frame.h"

namespace fetch
{

  namespace task
  {

    class WorkTask : public fetch::Task
    { public:
        unsigned int config(Agent *d) {return 1;} //success

        virtual void alloc_output_queues(Agent *agent);
    };

    //class UpdateableWorkTask : public fetch::UpdateableTask
    //{ public:
    //    virtual unsigned int config(Agent *d) {}
    //    virtual unsigned int update(Agent *d) {}

    //    virtual static void alloc_output_queues(Agent *agent);
    //};

    template<typename TMessage>
    class OneToOneWorkTask : public WorkTask
    { public:
                unsigned int run(Agent *d);
        virtual unsigned int work(Agent *agent, TMessage *dst, TMessage *src) = 0;
    };

    template<typename TMessage>
    class InPlaceWorkTask : public WorkTask
    { public:
                unsigned int run(Agent *d);
        virtual unsigned int work(Agent *agent, TMessage *src) = 0;
    };


// 
// Implementation
//

    template<typename TMessage>
    unsigned int
    OneToOneWorkTask<TMessage>::run(Agent *d)
    { unsigned int sts = 0; // success=0, fail=1
      asynq *qsrc = d->_in->contents[0],
            *qdst = d->_out->contents[0];
      TMessage *fsrc =  (TMessage*)Asynq_Token_Buffer_Alloc(qsrc),
               *fdst =  (TMessage*)Asynq_Token_Buffer_Alloc(qdst);
      size_t nbytes_in  = qsrc->q->buffer_size_bytes,
             nbytes_out = qdst->q->buffer_size_bytes,
             sz;
      
      while( !d->is_stopping() && Asynq_Pop(qsrc, (void**)&fsrc, nbytes_in) )
        { //debug("In  OneToOneWorkTask::run - just popped\r\n");
          nbytes_in = fsrc->size_bytes();
          fsrc->format(fdst);
          goto_if_fail(
            work(d,fdst,fsrc),
            WorkFunctionFailure);
          sz = fdst->size_bytes();                           
          nbytes_out = MAX( qdst->q->buffer_size_bytes, sz ); // XXX - awkward
          goto_if_fail(
            Asynq_Push_Timed( qdst, (void**)&fdst, nbytes_out, WORKER_DEFAULT_TIMEOUT ),
            OutputQueueTimeoutError);
        }      

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
    
    
    template<typename TMessage>
    unsigned int
    InPlaceWorkTask<TMessage>::run(Agent *d)
    { unsigned int sts = 0; // success=0, fail=1
      asynq *qsrc = d->_in->contents[0],
            *qdst = d->_out->contents[0];
      TMessage *fsrc =  (TMessage*)Asynq_Token_Buffer_Alloc(qsrc);
      size_t nbytes_in  = qsrc->q->buffer_size_bytes;
      
      while(!d->is_stopping() && Asynq_Pop(qsrc, (void**)&fsrc, nbytes_in) )
        { //debug("In  OneToOneWorkTask::run - just popped\r\n");
          nbytes_in = fsrc->size_bytes();
          goto_if_fail(
            work(d,fsrc),
            WorkFunctionFailure);                           
          nbytes_in = MAX( qdst->q->buffer_size_bytes, nbytes_in ); // XXX - awkward
          goto_if_fail(
            Asynq_Push_Timed( qdst, (void**)&fsrc, nbytes_in, WORKER_DEFAULT_TIMEOUT ),
            OutputQueueTimeoutError);
        }
      

    Finalize:
      Asynq_Token_Buffer_Free(fsrc);
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