#pragma once
#include "stdafx.h"
#include "agent.h"

namespace fetch
{
  typedef void Agent;

  class Task
  { public:
    virtual unsigned int config(Agent *d) = 0; // return 1 on sucess, 0 otherwise
    virtual unsigned int run(Agent *d)    = 0; // return 1 on sucess, 0 otherwise 
  
    DWORD WINAPI thread_main(LPVOID lpParam);
  }
}
