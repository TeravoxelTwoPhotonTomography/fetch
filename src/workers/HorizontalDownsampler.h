/*
 * HorizontalDownsampler.h
 *
 *  Created on: Apr 23, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once

#include "WorkAgent.h"
#include "WorkTask.h"
#include "workers.pb.h"

namespace fetch
{

  namespace task
  {
    
    class HorizontalDownsampler : public OneToOneWorkTask<Frame>
    { public:
        unsigned int reshape(IDevice *d, Frame *dst);
        unsigned int work(IDevice *agent, Frame *dst, Frame *src);
    };    
  }

  bool operator==(const cfg::worker::HorizontalDownsample& a, const cfg::worker::HorizontalDownsample& b);
  bool operator!=(const cfg::worker::HorizontalDownsample& a, const cfg::worker::HorizontalDownsample& b);

  namespace worker
  {
    typedef WorkAgent<task::HorizontalDownsampler,cfg::worker::HorizontalDownsample> HorizontalDownsampleAgent;
  }

}
