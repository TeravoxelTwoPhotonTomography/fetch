/*
 * FrameAverage.h
 *
 *  Created on: Apr 22, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#ifndef FRAMEAVERAGE_H_
#define FRAMEAVERAGE_H_

#include "WorkAgent.h"
#include "WorkTask.h"

namespace fetch
{

  namespace task
  {
   
    class FrameAverage : public WorkTask
    { public:
        unsigned int run(Agent* agent);
    };
  }

  namespace worker
  {
    typedef WorkAgent<task::FrameAverage,int> FrameAverageAgent;
  }

}

#endif /* FRAMEAVERAGE_H_ */