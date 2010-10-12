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
      :ZPiezoBase<Config>(agent)
      ,daq(agent,"ZPiezo")
    {
    }

    NIDAQZPiezo::NIDAQZPiezo( Agent *agent, Config *cfg )
      :ZPiezoBase<Config>(agent,cfg)
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

    void NIDAQZPiezo::computeConstWaveform(float64 z_um,  float64 *data, int n )
    {
      f64 off = z_um * _config->um2v();
      for(int i=0;i<n;++i)
        data[i] = off; // linear ramp from off to off+A
    }

    void NIDAQZPiezo::computeRampWaveform(float64 z_um,  float64 *data, int n )
    {
      f64   A = _config->um_step() * _config->um2v(),
          off = z_um * _config->um2v();
      for(int i=0;i<n;++i)
        data[i] = A*(i/(N-1))+off; // linear ramp from off to off+A
    }

    //
    // Simulate ZPiezo
    //

    SimulatedZPiezo::SimulatedZPiezo( Agent *agent )
      :ZPiezoBase<Config>(agent)
    {}

    SimulatedZPiezo::SimulatedZPiezo( Agent *agent, Config *cfg )
      :ZPiezoBase<Config>(agent,cfg)
    {}

    void SimulatedZPiezo::computeConstWaveform( float64 z_um, float64 *data, int n )
    {
      memset(data,0,sizeof(float64)*n);
    }

    void SimulatedZPiezo::computeRampWaveform( float64 z_um, float64 *data, int n )
    {
      memset(data,0,sizeof(float64)*n);
    }

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
 
    void ZPiezo::_set_config( Config IN *cfg )
    {
      _nidaq->_set_config(cfg->mutable_nidaq());
      _simulated->_set_config(cfg->mutable_simulated());;
      _config = cfg;
      setKind(cfg->kind());
    }

    void ZPiezo::_set_config( const Config &cfg )
    {
      cfg::device::ZPiezo_ZPiezoType kind = cfg.kind();
      _config->set_kind(kind);
      setKind(kind);
      switch(kind)
      {    
      case cfg::device::ZPiezo_ZPiezoType_NIDAQ:
        _nidaq->_set_config(cfg.nidaq());
        break;
      case cfg::device::ZPiezo_ZPiezoType_Simulated:    
        _simulated->_set_config(cfg.simulated());
        break;
      default:
        error("Unrecognized kind() for ZPiezo.  Got: %u\r\n",(unsigned)kind);
      }
    }

  }
}
