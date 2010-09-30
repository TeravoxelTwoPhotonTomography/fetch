#pragma once

#include "WorkTask.h"

namespace fetch {
namespace task {
  
class FrameInvert :
  public InPlaceWorkTask<Frame>
  {
  public:
        unsigned int work(Agent *agent, Frame *fsrc);
        
        virtual void alloc_output_queues(Agent *agent);

  };

} //namespace task

namespace worker {
  typedef WorkAgent<task::FrameInvert,int> FrameInvertAgent;
} //namespace worker

} //namespace fetch