/*
 * ZPiezo.cpp
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
#include "StdAfx.h"
#include "ZPiezo.h"

namespace fetch
{ namespace device
  {

    ZPiezo::ZPiezo(void)
           : NIDAQAgent("ZPiezo")
    {}

    ZPiezo::ZPiezo(const Config &cfg)
           : NIDAQAgent("ZPiezo")
    { config.MergeFrom(cfg);
    }
  
  }
}
