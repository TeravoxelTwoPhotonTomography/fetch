#include "common.h"
#include "task.h"

namespace fetch
{


  DWORD WINAPI
    Task::thread_main(LPVOID lpParam)
    {
      DWORD result;
      IDevice *dc   = (IDevice *) lpParam;
      Agent   *a    = dc->_agent;
      Task    *task = a->_task;
      result =  task->run(dc);
      a->_last_run_result=result;
      if(a->_is_running)
        a->stop_nowait(); // ensures thread gets cleaned up if a->stop() wasn't explicitly called.
      // Note to self: should probably take care of any
      //               cleanup in the Agent::stop() function
      return result;  
//Error:      
//      error("While ending the task, something went wrong.\r\n");
//      return 2; //failure - shouldn't get here.  
    }

// VTREF
//    Queries an instances virtual table and returns the address of the
//    function at the i'th entry.
//
// USAGE
//    <e> expression evaluating to a pointer to an instance of a class with virtual methods.
//
//    <i> the index of the virtual method for which to get the address.
#define VTREF(e,i) ( (void*) (((size_t*) *(size_t*)(e))[i]) )

// Task::eq
//    Defines an equivilance class over objects deriving from Task.
  bool                               //  Checks to make sure
  Task::eq(Task *a,Task *b)          //  config and run functions
  {                                  //  refer to the same address.
#if 0
    debug("a[0] = 0x%p.\r\n",VTREF(a,0));
    debug("b[0] = 0x%p.\r\n",VTREF(b,0));
    debug("a[1] = 0x%p.\r\n",VTREF(a,1));
    debug("b[1] = 0x%p.\r\n",VTREF(b,1));
#endif
  
    return VTREF(a,0)==VTREF(b,0) && 
           VTREF(a,1)==VTREF(b,1);
  }

}//end namespace fetch
