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

#include "WorkTask.h"

namespace fetch
{

  namespace task
  {

    class ResonantWrap : public fetch::task::OneToOneWorkTask<Frame>
    {
      public:
        unsigned int work(Agent *agent, Frame *dst, Frame *src);
    };

  } // namespace task

  namespace worker {

    typedef WorkAgent<task::ResonantWrap,float> ResonantWrapAgent;

  }  // namespace worker

}

