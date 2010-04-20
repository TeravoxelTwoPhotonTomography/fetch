#pragma once
#include "stdafx.h"
#include "agent.h"

//
// A Task instance should not carry any perminant state.
//
// That is, when an Task instance is not part of a 
// running Agent, it has no guaranteed lifetime.
//

namespace fetch
{
  typedef void Agent;

  class Task
  { public:
    virtual unsigned int config(Agent *d) = 0; // return 1 on success, 0 otherwise
    virtual unsigned int run(Agent *d)    = 0; // return 1 on success, 0 otherwise
  
    DWORD WINAPI thread_main(LPVOID lpParam);

    static bool eq(Task *a, Task *b); // a eq b iff addresses of config and run virtual functions are the same
  };

  class UpdateableTask: public Task
  { public:
    virtual unsigned int update(Agent *d) = 0; //return 1 on success, 0 otherwise

    static bool eq(UpdateableTask *a, UpdateableTask *b); // a eq b iff addresses of virtual functions are the same
  };
}
