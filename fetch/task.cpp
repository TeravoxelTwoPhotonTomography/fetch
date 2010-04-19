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

  static bool                        // Checks to make sure
    Task::eq(Task *a,Task *b)        //  config and run functions
  { return VTREF(a,0)==VTREF(b,0) && //  refer to the same address.
           VTREF(a,1)==VTREF(b,1)    //
  }
}
