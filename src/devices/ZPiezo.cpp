/*
 * ZPiezo.cpp
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
#include "StdAfx.h"
#include "ZPiezo.h"

namespace fetch
{ namespace device
  {
    //
    // NIDAQ ZPiezo
    //

    NIDAQZPiezo::NIDAQZPiezo( Agent *agent )
      :ZPiezoBase<cfg::device::NIDAQZPiezo>(agent)
      ,daq(agent,"ZPiezo")
    {
    }

    NIDAQZPiezo::NIDAQZPiezo( Agent *agent, Config *cfg )
      :ZPiezoBase<cfg::device::NIDAQZPiezo>(agent,cfg)
      ,daq(agent,"ZPiezo")
    {
    }

    unsigned int NIDAQZPiezo::attach()
    {
      return daq.attach();
    }

    unsigned int NIDAQZPiezo::detach()
    {
      return daq.detach();
    }

    //
    // Simulate ZPiezo
    //

    SimulatedZPiezo::SimulatedZPiezo( Agent *agent )
      :ZPiezoBase<f64>(agent)
    {
      *_config=0.0;
    }

    SimulatedZPiezo::SimulatedZPiezo( Agent *agent, Config *cfg )
      :ZPiezoBase<f64>(agent,cfg)
    {}

    //
    // ZPiezo
    //

    ZPiezo::ZPiezo( Agent *agent )
      :ZPiezoBase<cfg::device::ZPiezo>(agent)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_izpiezo(NULL)
    {
      setKind(_config->kind());
    }

    ZPiezo::ZPiezo( Agent *agent, Config *cfg )
      :ZPiezoBase<cfg::device::ZPiezo>(agent,cfg)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_izpiezo(NULL)
    {
      setKind(cfg->kind());
    }

    ZPiezo::~ZPiezo()
    {
      if(_nidaq)     { delete _nidaq;     _nidaq=NULL; }
      if(_simulated) { delete _simulated; _simulated=NULL; }
    }

    void ZPiezo::setKind( Config::ZPiezoType kind )
    {
      switch(kind)
      {    
      case cfg::device::ZPiezo_ZPiezoType_NIDAQ:
        if(!_nidaq)
          _nidaq = new NIDAQZPiezo(_agent,_config->mutable_nidaq());
        _idevice  = _nidaq;
        _izpiezo = _nidaq;
        break;
      case cfg::device::ZPiezo_ZPiezoType_Simulated:    
        if(!_simulated)
          _simulated = new SimulatedZPiezo(_agent);
        _idevice  = _simulated;
        _izpiezo = _simulated;
        break;
      default:
        error("Unrecognized kind() for ZPiezo.  Got: %u\r\n",(unsigned)kind);
      }
    }

    void ZPiezo::set_config( NIDAQZPiezo::Config *cfg )
    {
      Guarded_Assert(_nidaq);
      _nidaq->set_config(cfg);
    }

    void ZPiezo::set_config_nowait( SimulatedZPiezo::Config *cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config_nowait(cfg);
    }

    void ZPiezo::set_config_nowait( NIDAQZPiezo::Config *cfg )
    {
      Guarded_Assert(_nidaq);
      _nidaq->set_config_nowait(cfg);
    }

    void ZPiezo::set_config( SimulatedZPiezo::Config *cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config(cfg);
    }
    unsigned int ZPiezo::attach()
    {
      Guarded_Assert(_idevice);
      return _idevice->attach();
    }

    unsigned int ZPiezo::detach()
    {
      Guarded_Assert(_idevice);
      return _idevice->detach();
    }  

  }
}
