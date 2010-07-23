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

#define DEFAULT_LINEARSCANMIRROR_VPP                   0.1   // V - peak-to-peak
#define DEFAULT_LINEARSCANMIRROR_V_MAX                10.0   // V - Maximum permissible value
#define DEFAULT_LINEARSCANMIRROR_V_MIN               -10.0   // V - Minimum permissible value
#define DEFAULT_LINEARSCANMIRROR_CHANNEL        "/Dev1/ao0"  // DAQ terminal: should be connected to command input on galvo controller

#define LINEAR_SCAN_MIRROR__MAX_CHAN_STRING             32

namespace fetch
{

  namespace device
  {

    class LinearScanMirror : public NIDAQAgent
    {
    public:
      struct Config
      { f64         vpp;                                          // V - peak-to-peak
        f64         v_lim_max;                                    // V - Maximum permissible value
        f64         v_lim_min;                                    // V - Minimum permissible value
        char        channel [LINEAR_SCAN_MIRROR__MAX_CHAN_STRING];// DAQ terminal: should be connected to input on galvo controller
        
        Config()
        : vpp        (DEFAULT_LINEARSCANMIRROR_VPP),
          v_lim_max  (DEFAULT_LINEARSCANMIRROR_V_MAX),
          v_lim_min  (DEFAULT_LINEARSCANMIRROR_V_MIN)
        { strncpy(channel,DEFAULT_LINEARSCANMIRROR_CHANNEL,sizeof(DEFAULT_LINEARSCANMIRROR_CHANNEL));
        }
        
      };

      Config     config;

    public:
      LinearScanMirror();

      /* TODO: add methods to change vpp on the fly*/
    };

  }

}
