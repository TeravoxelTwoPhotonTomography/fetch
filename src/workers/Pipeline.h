/*
 * Pipeline.h
 *
 *  Created on: Feb 18, 2013
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "WorkAgent.h"
#include "WorkTask.h"
#include "workers.pb.h"

namespace fetch
{

  namespace task
  {
   
    class Pipeline : public WorkTask
    { public:
        unsigned int run(IDevice* dc);
    };
  }
  bool operator==(const cfg::worker::Pipeline& a, const cfg::worker::Pipeline& b);
  bool operator!=(const cfg::worker::Pipeline& a, const cfg::worker::Pipeline& b);
  namespace worker
  {
    class PipelineAgent:public WorkAgent<task::Pipeline,cfg::worker::Pipeline>
    {   unsigned scan_rate_Hz_;
        unsigned sample_rate_Mhz_;
      public:
        PipelineAgent();
        PipelineAgent(Config *config);
        unsigned scan_rate_Hz();
        unsigned sample_rate_Mhz();

        void set_scan_rate_Hz(unsigned v);
        void set_sample_rate_MHz(unsigned v);
    };
  }

}

#endif /* PIPELINE_H_ */
