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

namespace fetch {
  namespace device {

    //
    // NIDAQLinearScanMirror
    //

    NIDAQLinearScanMirror::NIDAQLinearScanMirror(Agent *agent)
      :LSMBase<cfg::device::LinearScanMirror>(agent)
      ,daq(agent,"NIDAQLinearScanMirror")
    {}

    NIDAQLinearScanMirror::NIDAQLinearScanMirror(Agent *agent,Config *cfg)
      :LSMBase<cfg::device::LinearScanMirror>(agent,cfg)
      ,daq(agent,"NIDAQLinearScanMirror")
    {}

    //
    // SimulatedLinearScanMirror
    //

    SimulatedLinearScanMirror::SimulatedLinearScanMirror( Agent *agent )
      :LSMBase<f64>(agent)
    { *_config=0.0;
    }

    SimulatedLinearScanMirror::SimulatedLinearScanMirror( Agent *agent, Config *cfg )
      :LSMBase<f64>(agent,cfg)
    {}

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

    void LinearScanMirror::set_config( NIDAQLinearScanMirror::Config *cfg )
    {
      Guarded_Assert(_nidaq);
      _nidaq->set_config(cfg);
    }

    void LinearScanMirror::set_config( SimulatedLinearScanMirror::Config *cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config(cfg);
    }

    void LinearScanMirror::set_config_nowait( NIDAQLinearScanMirror::Config *cfg )
    {
      Guarded_Assert(_nidaq);
      _nidaq->set_config_nowait(cfg);
    }

    void LinearScanMirror::set_config_nowait( SimulatedLinearScanMirror::Config *cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config_nowait(cfg);
    }

    //end namespace fetch::device
  }
}