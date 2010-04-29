#include "stdafx.h"
#include "task.h"

namespace fetch
{
  DWORD WINAPI
    Task::thread_main(LPVOID lpParam)
    { DWORD result;
      Agent *d    = (Agent*) lpParam;
      Task  *task = d->task;
      result = task->run(d);
      // Transition back to stop state when run returns
      d->lock();
      d->_is_running = 0;  
      CloseHandle(d->thread);
      d->thread = INVALID_HANDLE_VALUE;
      d->lock();
      
      return result;  
    }

#define VTREF(e,i) ((void*)(((size_t*) *(size_t*)&(e))[i]))

  bool                               // Checks to make sure
  Task::eq(Task *a,Task *b)          //  config and run functions
  { return VTREF(a,0)==VTREF(b,0) && //  refer to the same address.
           VTREF(a,1)==VTREF(b,1);   //
  }

  // * DELETED
  // * - Made "Updateable" aspect an interface so that an updatable task
  // *   inherits from task and IUpdate.
  // * - This means the update function won't get checked on a Task::eq check.
  // * + The equivilence function is static...so we could define eq...
  // * ? What's the vtable structure after multiple inheritance?
  // * - I think I'd have to compose the eq operators with each operator working on
  // *   it's own interface type.
  // I don't think I actually need this operator; the Task equivelence operator is
  // sufficient.
  //bool
  //UpdateableTask::eq(UpdateableTask *a, UpdateableTask *b)
  //{ return VTREF(a,0)==VTREF(b,0) && // config
  //         VTREF(a,1)==VTREF(b,1) && // run
  //         VTREF(a,2)==VTREF(b,2);   // update
  //}
  
}//end namespace fetch
