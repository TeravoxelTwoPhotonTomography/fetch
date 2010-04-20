/*
 * LinearScanMirror.h
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#pragma once
#include "NIDAQAgent.h"
#include "agent.h"

#define DEFAULT_LINEARSCANMIRROR_VPP                  10.0   // V - peak-to-peak
#define DEFAULT_LINEARSCANMIRROR_V_MAX                10.0   // V - Maximum permissible value
#define DEFAULT_LINEARSCANMIRROR_V_MIN               -10.0   // V - Minimum permissible value
#define DEFAULT_LINEARSCANMIRROR_CHANNEL        "/Dev1/ao0"  // DAQ terminal: should be connected to command input on galvo controller

#define LINEAR_SCAN_MIRROR__MAX_CHAN_STRING             32
#define LINEAR_SCAN_MIRROR__DEFAULT_CONFIG \
        { DEFAULT_LINEARSCANMIRROR_VPP,\
          DEFAULT_LINEARSCANMIRROR_V_MAX,\
          DEFAULT_LINEARSCANMIRROR_V_MIN,\
          DEFAULT_LINEARSCANMIRROR_CHANNEL,\
        }

namespace fetch
{

  namespace device
  {

    class LinearScanMirror : public NIDAQAgent
    {
    public:
      typedef struct _t_linear_scan_mirror_config
      { f64         vpp;                                          // V - peak-to-peak
        f64         v_lim_max;                                    // V - Maximum permissible value
        f64         v_lim_min;                                    // V - Minimum permissible value
        char        channel [LINEAR_SCAN_MIRROR__MAX_CHAN_STRING];// DAQ terminal: should be connected to input on galvo controller
      } Config;

      Config     config;

    public:
      LinearScanMirror();

      /* TODO: add methods to change vpp on the fly*/
    };

  }

}
