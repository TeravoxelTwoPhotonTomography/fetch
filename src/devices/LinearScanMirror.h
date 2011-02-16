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

      virtual double getAmplitudeVolts() = 0;
      virtual void   setAmplitudeVolts(double vpp) = 0;
      virtual void   setAmplitudeVoltsNoWait(double vpp) = 0;
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

      virtual unsigned int on_attach() {return daq.on_attach();}
      virtual unsigned int on_detach() {return daq.on_detach();}
      
      virtual void computeSawtooth(float64 *data, int n);

      virtual IDAQChannel* physicalChannel() {return &daq;}

      virtual double getAmplitudeVolts() {return _config->vpp();}
      virtual void   setAmplitudeVolts(double vpp) {Config c = get_config(); c.set_vpp(vpp); set_config(c);}
      virtual void   setAmplitudeVoltsNoWait(double vpp) {Config c = get_config(); c.set_vpp(vpp); Guarded_Assert_WinErr(set_config_nowait(c));}

    };

    class SimulatedLinearScanMirror : public LSMBase<cfg::device::SimulatedLinearScanMirror>
    {
      SimulatedDAQChannel _chan;
    public:
      SimulatedLinearScanMirror(Agent *agent);
      SimulatedLinearScanMirror(Agent *agent, Config *cfg);

      virtual unsigned int on_attach() {return 0;}
      virtual unsigned int on_detach() {return 0;}

      virtual void computeSawtooth(float64 *data, int n);

      virtual IDAQChannel* physicalChannel() {return &_chan;}

      virtual double getAmplitudeVolts() {return _config->val();}
      virtual void   setAmplitudeVolts(double vpp) {Config c = get_config(); c.set_val(vpp); set_config(c);}
      virtual void   setAmplitudeVoltsNoWait(double vpp) {Config c = get_config(); c.set_val(vpp); Guarded_Assert_WinErr(set_config_nowait(c));}
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

     virtual unsigned int on_attach() {return _idevice->on_attach();}
     virtual unsigned int on_detach() {return _idevice->on_detach();}
     void _set_config( Config IN *cfg );
     void _set_config( const Config &cfg );

     virtual void computeSawtooth(float64 *data, int n);

     virtual IDAQChannel* physicalChannel() {return _ilsm->physicalChannel();}

     virtual double getAmplitudeVolts() {return _ilsm->getAmplitudeVolts();}
     virtual void   setAmplitudeVolts(double vpp) {_ilsm->setAmplitudeVolts(vpp);}
     virtual void   setAmplitudeVoltsNoWait(double vpp) {_ilsm->setAmplitudeVoltsNoWait(vpp);}
   };

   //end namespace fetch::device
  }
}
