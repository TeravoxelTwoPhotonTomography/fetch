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


#include "LinearScanMirror.h"

namespace fetch {

  bool operator==(const cfg::device::NIDAQLinearScanMirror& a, const cfg::device::NIDAQLinearScanMirror& b)         {return equals(&a,&b);}
  bool operator==(const cfg::device::SimulatedLinearScanMirror& a, const cfg::device::SimulatedLinearScanMirror& b) {return equals(&a,&b);}
  bool operator==(const cfg::device::LinearScanMirror& a, const cfg::device::LinearScanMirror& b)                   {return equals(&a,&b);}
  bool operator!=(const cfg::device::NIDAQLinearScanMirror& a, const cfg::device::NIDAQLinearScanMirror& b)         {return !(a==b);}
  bool operator!=(const cfg::device::SimulatedLinearScanMirror& a, const cfg::device::SimulatedLinearScanMirror& b) {return !(a==b);}
  bool operator!=(const cfg::device::LinearScanMirror& a, const cfg::device::LinearScanMirror& b)                   {return !(a==b);}

  namespace device {

    //
    // NIDAQLinearScanMirror
    //

    NIDAQLinearScanMirror::NIDAQLinearScanMirror(Agent *agent)
      :LSMBase<Config>(agent)
      ,daq(agent,"fetch_NIDAQLinearScanMirror")
      ,_pchan(_config->ao_channel())
    {}

    NIDAQLinearScanMirror::NIDAQLinearScanMirror(Agent *agent,Config *cfg)
      :LSMBase<Config>(agent,cfg)
      ,daq(agent,"fetch_NIDAQLinearScanMirror")
      ,_pchan(cfg->ao_channel())
    {}

    void NIDAQLinearScanMirror::computeSawtooth( float64 *data, int flyback, int n )
    { int i;
      double N = flyback;
      float64 A = _config->vpp();
      for(i=0;i<flyback;++i)
        data[i] = A*((i/N)-0.5); // linear ramp from -A/2 to A/2
      for(;i<n;++i)
        data[i]=data[0];         // at end of wave, head back to the starting position
    }

    //
    // SimulatedLinearScanMirror
    //

    SimulatedLinearScanMirror::SimulatedLinearScanMirror( Agent *agent )
      :LSMBase<Config>(agent)
      ,_chan(agent,"LSM")
      ,_pchan("simulated")
    {}

    SimulatedLinearScanMirror::SimulatedLinearScanMirror( Agent *agent, Config *cfg )
      :LSMBase<Config>(agent,cfg)
      ,_chan(agent,"LSM")
      ,_pchan("simulated")
    {}

    void SimulatedLinearScanMirror::computeSawtooth( float64 *data, int flyback, int n )
    {
      memset(data,0,sizeof(float64)*n);
    }

    //
    // LinearScanMirror
    //

    LinearScanMirror::LinearScanMirror( Agent *agent )
      :LSMBase<cfg::device::LinearScanMirror>(agent)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_ilsm(NULL)
    {
       setKind(_config->kind());
    }

    LinearScanMirror::LinearScanMirror( Agent *agent, Config *cfg )
      :LSMBase<cfg::device::LinearScanMirror>(agent,cfg)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_ilsm(NULL)
    {
      setKind(_config->kind());
    }

    LinearScanMirror::~LinearScanMirror()
    {
      if(_nidaq){delete _nidaq; _nidaq=NULL;}
      if(_simulated){delete _simulated; _simulated=NULL;}
    }

    void LinearScanMirror::setKind( Config::LinearScanMirrorType kind )
    {
      switch(kind)
      {
      case cfg::device::LinearScanMirror_LinearScanMirrorType_NIDAQ:
        if(!_nidaq)
          _nidaq = new NIDAQLinearScanMirror(_agent,_config->mutable_nidaq());
        _idevice  = _nidaq;
        _ilsm = _nidaq;
        break;
      case cfg::device::LinearScanMirror_LinearScanMirrorType_Simulated:
        if(!_simulated)
          _simulated = new SimulatedLinearScanMirror(_agent);
        _idevice  = _simulated;
        _ilsm = _simulated;
        break;
      default:
        error("Unrecognized kind() for LinearScanMirror.  Got: %u\r\n",(unsigned)kind);
      }
    }

    void LinearScanMirror::_set_config( Config IN *cfg )
    {
      setKind(cfg->kind());
      Guarded_Assert(_nidaq||_simulated); // at least one device was instanced
      if(_nidaq)     _nidaq->_set_config(cfg->mutable_nidaq());
      if(_simulated) _simulated->_set_config(cfg->mutable_simulated());
      _config = cfg;
    }

    void LinearScanMirror::_set_config( const Config &cfg )
    {
      cfg::device::LinearScanMirror_LinearScanMirrorType kind = cfg.kind();                //[ ] who calls this and where does _config get updated?  what should be the const behavior?
      _config->set_kind(kind);
      setKind(kind);
      switch(kind)
      {
      case cfg::device::LinearScanMirror_LinearScanMirrorType_NIDAQ:
        _nidaq->_set_config(const_cast<Config&>(cfg).mutable_nidaq());
        break;
      case cfg::device::LinearScanMirror_LinearScanMirrorType_Simulated:
        _simulated->_set_config(cfg.simulated());
        break;
      default:
        error("Unrecognized kind() for LinearScanMirror.  Got: %u\r\n",(unsigned)kind);
      }
    }

    void LinearScanMirror::computeSawtooth( float64 *data, int flyback, int n )
    {
      _ilsm->computeSawtooth(data,flyback,n);
    }

    //end namespace fetch::device
  }
}