/*
 * Probe.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: clackn
 */

#include "common.h"
#include "probe.h"
#include "DAQChannel.h"
#include "object.h"

#include "../util/util-nidaqmx.h"

#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )   goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  bool operator==(const cfg::device::NIDAQProbe& a, const cfg::device::NIDAQProbe& b)         {return equals(&a,&b);}
  bool operator==(const cfg::device::SimulatedProbe& a, const cfg::device::SimulatedProbe& b) {return equals(&a,&b);}
  bool operator==(const cfg::device::Probe& a, const cfg::device::Probe& b)                   {return equals(&a,&b);}
  bool operator!=(const cfg::device::NIDAQProbe& a, const cfg::device::NIDAQProbe& b)         {return !(a==b);}
  bool operator!=(const cfg::device::SimulatedProbe& a, const cfg::device::SimulatedProbe& b) {return !(a==b);}
  bool operator!=(const cfg::device::Probe& a, const cfg::device::Probe& b)                   {return !(a==b);}


  namespace device
  {        

    //
    // NIDAQ Probe
    //

    NIDAQProbe::NIDAQProbe( Agent *agent )
      :ProbeBase<Config>(agent)
      ,daq(agent,"fetch_Probe")
      ,_ai(_config->ai_channel())
    {
    }

    NIDAQProbe::NIDAQProbe( Agent *agent, Config *cfg )
      :ProbeBase<Config>(agent,cfg)
      ,daq(agent,"fetch_Probe")
      ,_ai(cfg->ai_channel())
    {
    }

    unsigned int NIDAQProbe::on_attach()
    {
      unsigned int sts = daq.on_attach();
      if(sts==0) // 0 is success
        Bind();
      return sts;
    }

    unsigned int NIDAQProbe::on_detach()
    {
      return daq.on_detach();
    }


    float
    NIDAQProbe::read()
    { float64 v=0;
      int32 n=0;
      DAQERR(DAQmxReadAnalogF64(daq.daqtask,1,10000,DAQmx_Val_GroupByChannel,&v,1,&n,NULL));
      return v;
    }
    
    void
    NIDAQProbe::Bind(void)
    { DAQERR( DAQmxClearTask(daq.daqtask) );
      DAQERR( DAQmxCreateTask(daq._daqtaskname,&daq.daqtask));
      DAQERR( DAQmxCreateAIVoltageChan(daq.daqtask,
        _config->ai_channel().c_str(),
        "probe-input",
        DAQmx_Val_Cfg_Default,
        (float64)_config->v_min(),
        (float64)_config->v_max(),
        DAQmx_Val_Volts,
        NULL
        ));
      DAQERR( DAQmxStartTask( daq.daqtask ) ); // go ahead and start it
    }

    //
    // Simulate Probe
    //

    SimulatedProbe::SimulatedProbe( Agent *agent )
      :ProbeBase<Config>(agent)
    {}

    SimulatedProbe::SimulatedProbe( Agent *agent, Config *cfg )
      :ProbeBase<Config>(agent,cfg)
    {}

    //
    // Probe
    //

    Probe::Probe( Agent *agent )
      :ProbeBase<cfg::device::Probe>(agent)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_iprobe(NULL)
    {
      setKind(_config->kind());
    }

    Probe::Probe( Agent *agent, Config *cfg )
      :ProbeBase<cfg::device::Probe>(agent,cfg)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_iprobe(NULL)
    {
      setKind(cfg->kind());
    }

    Probe::~Probe()
    {
      if(_nidaq)     { delete _nidaq;     _nidaq=NULL; }
      if(_simulated) { delete _simulated; _simulated=NULL; }
    }

    void Probe::setKind( Config::ProbeType kind )
    {
      switch(kind)
      {    
      case cfg::device::Probe_ProbeType_NIDAQ:
        if(!_nidaq)
          _nidaq = new NIDAQProbe(_agent,_config->mutable_nidaq());
        _idevice  = _nidaq;
        _iprobe = _nidaq;
        break;
      case cfg::device::Probe_ProbeType_Simulated:    
        if(!_simulated)
          _simulated = new SimulatedProbe(_agent);
        _idevice  = _simulated;
        _iprobe = _simulated;
        break;
      default:
        error("Unrecognized kind() for Probe.  Got: %u\r\n",(unsigned)kind);
      }
    }

    void Probe::_set_config( Config IN *cfg )
    {
      setKind(cfg->kind());
      Guarded_Assert(_nidaq||_simulated); // at least one device was instanced
      if(_nidaq)     _nidaq->_set_config(cfg->mutable_nidaq());
      if(_simulated) _simulated->_set_config(cfg->mutable_simulated());;
      _config = cfg;
    }

    void Probe::_set_config( const Config &cfg )
    {
      cfg::device::Probe_ProbeType kind = cfg.kind();
      _config->set_kind(kind);
      setKind(kind);
      switch(kind)
      {    
      case cfg::device::Probe_ProbeType_NIDAQ:
        _nidaq->_set_config(const_cast<Config&>(cfg).mutable_nidaq());
        break;
      case cfg::device::Probe_ProbeType_Simulated:    
        _simulated->_set_config(cfg.simulated());
        break;
      default:
        error("Unrecognized kind() for Probe.  Got: %u\r\n",(unsigned)kind);
      }
    }

//end namespace fetch::device
  }
}
