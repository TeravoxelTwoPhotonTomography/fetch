/*
 *  Created on: 2013
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#pragma once

#include "WorkAgent.h"
#include "WorkTask.h"
#include "tasks.pb.h"

namespace fetch
{

  namespace task
  {
   
    class SurfaceFindWorker : public WorkTask
    { public:
        unsigned int run(IDevice* dc);
    };
  }
  bool operator==(const cfg::tasks::SurfaceFind& a, const cfg::tasks::SurfaceFind& b);
  bool operator!=(const cfg::tasks::SurfaceFind& a, const cfg::tasks::SurfaceFind& b);
  namespace worker
  {
    class SurfaceFindWorkerAgent:public WorkAgent<task::SurfaceFindWorker,cfg::tasks::SurfaceFind>
    { 
      unsigned last_found_;
    	unsigned any_found_;
      public:
        SurfaceFindWorkerAgent();
        SurfaceFindWorkerAgent(Config *config);
        void set(unsigned i);
        unsigned which();
        unsigned any();
        unsigned too_inside();   // every plan trips threshold
        unsigned too_outside();  // no planes trip threshold

        void reset();
    };
  }

}