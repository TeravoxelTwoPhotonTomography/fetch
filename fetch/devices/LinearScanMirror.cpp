/*
 * LinearScanMirror.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org
 */
 /*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#include "stdafx.h"
#include "LinearScanMirror.h"

namespace fetch
{

  namespace device
  {

    LinearScanMirror::LinearScanMirror()
      : NIDAQAgent("LinearScanMirror"),
        config(&_default_config)
    {}

    LinearScanMirror::LinearScanMirror( const Config &cfg )
      : NIDAQAgent("LinearScanMirror"),
        config(&_default_config)
    { config->CopyFrom(cfg);      
    }

    LinearScanMirror::LinearScanMirror( Config *cfg )
      : NIDAQAgent("LinearScanMirror"),
        config(cfg)
    {}

  }

}
