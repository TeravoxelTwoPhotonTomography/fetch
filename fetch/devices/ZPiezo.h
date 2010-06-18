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

#define DEFAULT_ZPIEZO_UM2V                0.025   // V/um - approximate micron to volts converstion (for control output). e.g. 1 um = 0.025 V = 25 mV
#define DEFAULT_ZPIEZO_UM_MAX               200   // um   - end of travel for a scan
#define DEFAULT_ZPIEZO_UM_MIN                0.0   // um   - beg of travel for a scan
#define DEFAULT_ZPIEZO_UM_STEP               0.5   // um   - step to take during a vertical scan
#define DEFAULT_ZPIEZO_V_MAX                12.0   // V    - Maximum permissible value
#define DEFAULT_ZPIEZO_V_MIN                -3.0   // V    - Minimum permissible value
#define DEFAULT_ZPIEZO_V_OFFSET              0.0   // V    - Voltage corresponding to the spatial zero point
#define DEFAULT_ZPIEZO_CHANNEL        "/Dev1/ao1"  // DAQ terminal: should be connected to command input on galvo controller

#define ZPIEZO_MIRROR__MAX_CHAN_STRING        32

namespace fetch 
{ namespace device 
  {

    class ZPiezo : public NIDAQAgent
    {
    public:
      ZPiezo(void);
      
    public:
      struct Config
      { f64 um2v;
        f64 um_max;
        f64 um_min;
        f64 um_step;
        f64 v_lim_max;
        f64 v_lim_min;
        f64 v_offset;
        char channel[ZPIEZO_MIRROR__MAX_CHAN_STRING];
        
        Config();      
      };
      
      Config config;
    };
      
  }
}