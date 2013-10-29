/*
 *  Created on: 2013
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#pragma once

#include "WorkAgent.h"
#include "WorkTask.h"
#include "workers.pb.h"

namespace fetch
{

  namespace task
  {
   
    class TripDetectWorker : public WorkTask
    { public:
        unsigned int run(IDevice* dc);
    };
  }
  bool operator==(const cfg::worker::TripDetect& a, const cfg::worker::TripDetect& b);
  bool operator!=(const cfg::worker::TripDetect& a, const cfg::worker::TripDetect& b);
  namespace worker
  {
    class TripDetectWorkerAgent:public WorkAgent<task::TripDetectWorker,cfg::worker::TripDetect>
    { 
      unsigned number_dark_frames_; // incremented for every dark frame
      public:
        TripDetectWorkerAgent();
        TripDetectWorkerAgent(Config *config);
        
        void inc();
        unsigned ok(); // returns 1 if number_of_dark_frames_ < threshold specified in config
        void reset();  // manually resets number_dark_frames_ to 0.
    };
  }
}