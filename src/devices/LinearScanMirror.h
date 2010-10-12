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
#include "DAQChannel.h"
#include "../agent.h"
#include "linear_scan_mirror.pb.h"
#include "object.h"

#define LINEAR_SCAN_MIRROR__MAX_CHAN_STRING             32

namespace fetch
{

  namespace device
  {

    class ILSM
    {
    public:      /* TODO: add methods to change vpp on the fly*/
      virtual void computeSawtooth(float64 *data, int n)=0;
      virtual IDAQChannel* physicalChannel() = 0;
    };

    template<class T>
    class LSMBase:public ILSM,public IConfigurableDevice<T>
    {
    public:
      LSMBase(Agent *agent)             :IConfigurableDevice<T>(agent) {}
      LSMBase(Agent *agent, Config* cfg):IConfigurableDevice<T>(agent,cfg) {}
    };

    class NIDAQLinearScanMirror : public LSMBase<cfg::device::NIDAQLinearScanMirror>
    {
      NIDAQChannel daq;
    public:
      NIDAQLinearScanMirror(Agent *agent);
      NIDAQLinearScanMirror(Agent *agent, Config *cfg);

      virtual unsigned int attach() {return daq.attach();}
      virtual unsigned int detach() {return daq.detach();}
      
      virtual void computeSawtooth(float64 *data, int n);

      virtual IDAQChannel* physicalChannel() {return &daq;}
    };

    class SimulatedLinearScanMirror : public LSMBase<cfg::device::SimulatedLinearScanMirror>
    {
      SimulatedDAQChannel _chan;
    public:
      SimulatedLinearScanMirror(Agent *agent);
      SimulatedLinearScanMirror(Agent *agent, Config *cfg);

      virtual unsigned int attach() {return 0;}
      virtual unsigned int detach() {return 0;}

      virtual void computeSawtooth(float64 *data, int n);

      virtual IDAQChannel* physicalChannel() {return &_chan;}
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

     virtual unsigned int attach() {return _idevice->attach();}
     virtual unsigned int detach() {return _idevice->detach();}
     void _set_config( Config IN *cfg );
     void _set_config( const Config &cfg );

     virtual void computeSawtooth(float64 *data, int n);

     virtual IDAQChannel* physicalChannel() {return _ilsm->physicalChannel();}
   };

   //end namespace fetch::device
  }
}
