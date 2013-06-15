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

#if 0 // PROFILING
#include "../util/timestream.h"
#define TS_OPEN(e,name)    timestream_t ts__##e=timestream_open(name)
#define TS_TIC(e)          timestream_tic(ts__##e)
#define TS_TOC(e)          timestream_toc(ts__##e)
#define TS_CLOSE(e)        timestream_close(ts__##e)
#define PROFILE(e,expr)   do{ (expr); TS_TOC(e); } while(0)
#else
#define TS_OPEN(e,name)
#define TS_TIC(e)
#define TS_TOC(e)
#define TS_CLOSE(e)
#define PROFILE(e,expr)   (expr)
#endif

#define countof(e) (sizeof(e)/sizeof(*e))

#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )   goto_if_fail( 0==DAQWRN(expr), Error)
#define PROFILE(e,expr)   do{ (expr); TS_TOC(e); } while(0)

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


    NIDAQProbe::~NIDAQProbe()
    {
    }

    unsigned int NIDAQProbe::on_attach()
    {
      unsigned int sts = 0;//daq.on_attach();

      // if(sts==0) // 0 is success
      //   Bind();
      return sts;
    }

    unsigned int NIDAQProbe::on_detach()
    {

      TS_CLOSE(1);
      return 0; //daq.on_detach();
    }


    float
    NIDAQProbe::read()
    { float64 v[1]={0},mean=0;
      int32 n=0;

//int32 DAQmxSendSoftwareTrigger (TaskHandle taskHandle, int32 triggerID);
      PROFILE(1,DAQERR(DAQmxReadAnalogF64(daq.daqtask,1,DAQmx_Val_WaitInfinitely,DAQmx_Val_GroupByChannel,v,countof(v),&n,NULL)));

      for(int i=0;i<countof(v);++i)
        mean+=v[i];
      return mean/countof(v);
    }

    void
    NIDAQProbe::Bind(void)
    { TS_TIC(1);
      PROFILE(1,DAQERR( DAQmxClearTask(daq.daqtask) ));
      PROFILE(1,DAQERR( DAQmxCreateTask(daq._daqtaskname,&daq.daqtask)));
      PROFILE(1,DAQERR( DAQmxCreateAIVoltageChan(daq.daqtask,
        _config->ai_channel().c_str(),
        "probe-input",
        DAQmx_Val_Diff , // could be configurable
        (float64)_config->v_min(),
        (float64)_config->v_max(),
        DAQmx_Val_Volts,
        NULL
        )));

      DAQERR(DAQmxSetSampTimingType(daq.daqtask,DAQmx_Val_OnDemand));
      //PROFILE(1,DAQERR( DAQmxCfgImplicitTiming(daq.daqtask,DAQmx_Val_HWTimedSinglePoint,1)));
      PROFILE(1,DAQERR( DAQmxStartTask( daq.daqtask ) )); // go ahead and start it
    }

    void
    NIDAQProbe::UnBind(void)
    { DAQmxWaitUntilTaskDone(daq.daqtask,1);
      DAQmxStopTask( daq.daqtask ); // go ahead and start it
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
