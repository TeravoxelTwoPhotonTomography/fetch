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
}
