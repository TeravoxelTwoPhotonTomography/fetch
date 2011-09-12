/*
 * ResonantWrap.h
 *
 *  Created on: May 17, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#pragma once

#include "workers.pb.h"
#include "WorkTask.h"

#define RESONANTUNWARPAGENT_DEFAULT_TIMEOUT         INFINITE

namespace fetch
{

  namespace task
  {

    class ResonantUnwarp : public fetch::task::OneToOneWorkTask<Frame>
    {
      public:
        unsigned int reshape(IDevice *d, Frame *dst);
        unsigned int work(IDevice *d, Frame *dst, Frame *src);
    };

  } // namespace task

  namespace worker {

    // Implementation is trivial
    class ResonantUnwarpAgent:public WorkAgent<task::ResonantUnwarp,cfg::worker::ResonantUnwarp>
    {
      public:
        ResonantUnwarpAgent();
        ResonantUnwarpAgent(Config *config);

        float duty();
        void setDuty(float v);
        int  setDutyNoWait(float v);
    };

  }  // namespace worker

}

