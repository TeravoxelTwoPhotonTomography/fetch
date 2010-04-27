/*
 * PixelWiseAverager.h
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

namespace fetch
{

  namespace task
  {

    typedef WorkAgent<PixelWiseAverager,int> PixelWiseAveragerAgent;

    class PixelWiseAverager : public OneToOneWorkTask
    { public:
        unsigned int work(PixelWiseAveragerAgent *agent, Frame *dst, Frame *src);
    };

  }

}
