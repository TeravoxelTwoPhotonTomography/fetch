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

    class ILSM
    {
    public:      /* TODO: add methods to change vpp on the fly*/
    };

    template<class T>
    class LSMBase:public ILSM,public IConfigurableDevice<T>
    {
    public:
      LSMBase(Agent *agent)             :IConfigurableDevice(agent) {}
      LSMBase(Agent *agent, Config* cfg):IConfigurableDevice(agent,cfg) {}
    };

    class NIDAQLinearScanMirror : public LSMBase<cfg::device::NIDAQLinearScanMirror>
    {
      NIDAQAgent daq;
    public:
      NIDAQLinearScanMirror(Agent *agent);
      NIDAQLinearScanMirror(Agent *agent, Config *cfg);

      virtual unsigned int attach() {return daq.attach();}
      virtual unsigned int detach() {return daq.detach();}
    };

    class SimulatedLinearScanMirror : public LSMBase<f64>
    {
    public:
      SimulatedLinearScanMirror(Agent *agent);
      SimulatedLinearScanMirror(Agent *agent, Config *cfg);

      virtual unsigned int attach() {return 0;}
      virtual unsigned int detach() {return 0;}
    };
   
   class LinearScanMirror:public LSMBase<cfg::device::LinearScanMirror>
   {
     NIDAQLinearScanMirror     *_nidaq;
     SimulatedLinearScanMirror *_simulated;
     IDevice *_idevice;
     ILSM    *_ilsm;
   public:
     LinearScanMirror(Agent *agent);
     LinearScanMirror(Agent *agent, Config *cfg);
     ~LinearScanMirror();

     void setKind(Config::LinearScanMirrorType kind);

     virtual void set_config(NIDAQLinearScanMirror::Config *cfg);
     virtual void set_config(SimulatedLinearScanMirror::Config *cfg);
     virtual void set_config_nowait(NIDAQLinearScanMirror::Config *cfg);
     virtual void set_config_nowait(SimulatedLinearScanMirror::Config *cfg);
   };

   //end namespace fetch::device
  }
}
