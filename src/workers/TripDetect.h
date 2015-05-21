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
  namespace device { class Microscope;}

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
      unsigned number_resets_;      // incremented for every reset of the pmt controller
      device::Microscope* microscope_;
      public:
        TripDetectWorkerAgent(device::Microscope* dc);
        TripDetectWorkerAgent(device::Microscope* dc,Config *config);
        
        void inc();
        unsigned ok(); // returns 1 if number_of_dark_frames_ < threshold specified in config
        unsigned cycle_pmts(); // turns the pmt's off and on again.  returns 1 if number_of_resets_ < threshold specified in config
        void reset();  // manually resets number_dark_frames_ to 0.
        void sig_stop(); // signal the microscope to stop the current task
    };
  }
}