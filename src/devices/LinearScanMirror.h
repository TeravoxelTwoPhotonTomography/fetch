/*
 * LinearScanMirror.h
 *
 *  Created on: Apr 20, 2010
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
#include "../agent.h"
#include "linear_scan_mirror.pb.h"
#include "object.h"

//#define DEFAULT_LINEARSCANMIRROR_VPP                   2.5   // V - peak-to-peak
//#define DEFAULT_LINEARSCANMIRROR_V_MAX                10.0   // V - Maximum permissible value
//#define DEFAULT_LINEARSCANMIRROR_V_MIN               -10.0   // V - Minimum permissible value
//#define DEFAULT_LINEARSCANMIRROR_CHANNEL        "/Dev1/ao0"  // DAQ terminal: should be connected to command input on galvo controller

#define LINEAR_SCAN_MIRROR__MAX_CHAN_STRING             32

namespace fetch
{

  namespace device
  {

    class LinearScanMirror
      : public NIDAQAgent,
        public Configurable<cfg::device::LinearScanMirror>
    {
    public:
      LinearScanMirror();
      LinearScanMirror(Config *cfg);

      /* TODO: add methods to change vpp on the fly*/
    };

  }

}
