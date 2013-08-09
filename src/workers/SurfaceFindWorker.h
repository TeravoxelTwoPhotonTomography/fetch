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
   
    class SurfaceFindWorker : public WorkTask
    { public:
        unsigned int run(IDevice* dc);
    };
  }
  bool operator==(const cfg::worker::SurfaceFindWorker& a, const cfg::worker::SurfaceFindWorker& b);
  bool operator!=(const cfg::worker::SurfaceFindWorker& a, const cfg::worker::SurfaceFindWorker& b);
  namespace worker
  {
    class SurfaceFindWorkerAgent:public WorkAgent<task::SurfaceFindWorker,cfg::worker::SurfaceFindWorker>
    {   unsigned last_found_;
    	unsigned any_found_;
      public:
        SurfaceFindWorkerAgent();
        SurfaceFindWorkerAgent(Config *config);
        unsigned which();
        unsigned any();

        void reset();
    };
  }

}

#endif /* PIPELINE_H_ */
