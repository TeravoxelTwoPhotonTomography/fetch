#pragma once

#include "WorkTask.h"

namespace fetch {
namespace task {
  
class FrameInvert :
  public InPlaceWorkTask<Frame>
  {
  public:
        unsigned int work(IDevice *dc, Frame *fsrc);
        
        virtual void alloc_output_queues(IDevice *dc);

  };

} //namespace task

namespace worker {
  typedef WorkAgent<task::FrameInvert> FrameInvertAgent;
} //namespace worker

} //namespace fetch