/*
 * ZPiezo.h
 *
 *  Created on: May 7, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
 /*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once
#include "NIDAQAgent.h"
#include "zpiezo.pb.h"
#include "object.h"

namespace fetch 
{ namespace device 
  {

    class ZPiezo
      : public NIDAQAgent,
        public Configurable<cfg::device::ZPiezo>
    {
    public:

      ZPiezo(void);
      ZPiezo(Config *cfg);
    };
      
  }
}