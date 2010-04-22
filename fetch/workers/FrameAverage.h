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

    typedef WorkAgent<FrameAverage,int> FrameAverageAgent;

    class FrameAverage : public UpdateableWorkTask
    { public:
        unsigned int run(FrameAverageAgent* agent);
    };

  }

}

#endif /* FRAMEAVERAGE_H_ */
