/*
* DAQ.cpp
*
*  Created on: Oct 10, 2010
*      Author: Nathan Clack <clackn@janelia.hhmi.org
*/
/*
* Copyright 2010 Howard Hughes Medical Institute.
* All rights reserved.
* Use is subject to Janelia Farm Research Campus Software Copyright 1.1
* license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
*/

#include "stdafx.h"
#include "daq.h"

namespace fetch {
  namespace device {

    //
    // NationalInstrumentsDAQ
    //

    NationalInstrumentsDAQ::NationalInstrumentsDAQ(Agent *agent)
      :DAQBase<cfg::device::DAQ>(agent)
    {}

    NationalInstrumentsDAQ::NationalInstrumentsDAQ(Agent *agent,Config *cfg)
      :DAQBase<cfg::device::DAQ>(agent,cfg)
    {}

    //
    // SimulatedDAQ
    //

    SimulatedDAQ::SimulatedDAQ( Agent *agent )
      :DAQBase<f64>(agent)
    { *_config=0.0;
    }

    SimulatedDAQ::SimulatedDAQ( Agent *agent, Config *cfg )
      :DAQBase<f64>(agent,cfg)
    {}

    //
    // DAQ
    //

    DAQ::DAQ( Agent *agent )
      :DAQBase<cfg::device::DAQ>(agent)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_ilsm(NULL)
    {
       setKind(_config->kind());
    }

    DAQ::DAQ( Agent *agent, Config *cfg )
      :DAQBase<cfg::device::DAQ>(agent,cfg)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_ilsm(NULL)
    {
      setKind(_config->kind());
    }

    DAQ::~DAQ()
    {
      if(_nidaq){delete _nidaq; _nidaq=NULL;}
      if(_simulated){delete _simulated; _simulated=NULL;}
    }

    void DAQ::setKind( Config::DAQKind kind )
    {
      switch(kind)
      {    
      case cfg::device::DAQ_DAQKind_NIDAQ:
        if(!_nidaq)
          _nidaq = new NationalInstrumentsDAQ(_agent,_config->mutable_nidaq());
        _idevice  = _nidaq;
        _ilsm = _nidaq;
        break;
      case cfg::device::DAQ_DAQKind_Simulated:
        if(!_simulated)
          _simulated = new SimulatedDAQ(_agent);
        _idevice  = _simulated;
        _ilsm = _simulated;
        break;
      default:
        error("Unrecognized kind() for DAQ.  Got: %u\r\n",(unsigned)kind);
      }
    }

    void DAQ::set_config( NationalInstrumentsDAQ::Config *cfg )
    {
      Guarded_Assert(_nidaq);
      _nidaq->set_config(cfg);
    }

    void DAQ::set_config( SimulatedDAQ::Config *cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config(cfg);
    }

    void DAQ::set_config_nowait( NationalInstrumentsDAQ::Config *cfg )
    {
      Guarded_Assert(_nidaq);
      _nidaq->set_config_nowait(cfg);
    }

    void DAQ::set_config_nowait( SimulatedDAQ::Config *cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config_nowait(cfg);
    }

    //end namespace fetch::device
  }
}

