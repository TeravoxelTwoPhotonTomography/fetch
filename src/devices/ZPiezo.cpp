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

#include "ZPiezo.h"

namespace fetch
{

  bool operator==(const cfg::device::NIDAQZPiezo& a, const cfg::device::NIDAQZPiezo& b)         {return equals(&a,&b);}
  bool operator==(const cfg::device::SimulatedZPiezo& a, const cfg::device::SimulatedZPiezo& b) {return equals(&a,&b);}
  bool operator==(const cfg::device::ZPiezo& a, const cfg::device::ZPiezo& b)                   {return equals(&a,&b);}

  bool operator!=(const cfg::device::NIDAQZPiezo& a, const cfg::device::NIDAQZPiezo& b)         {return !(a==b);}
  bool operator!=(const cfg::device::SimulatedZPiezo& a, const cfg::device::SimulatedZPiezo& b) {return !(a==b);}
  bool operator!=(const cfg::device::ZPiezo& a, const cfg::device::ZPiezo& b)                   {return !(a==b);}

  namespace device
  {
    //
    // NIDAQ ZPiezo
    //

    NIDAQZPiezo::NIDAQZPiezo( Agent *agent )
      :ZPiezoBase<Config>(agent)
      ,daq(agent,"fetch_ZPiezo")
      ,_ao(_config->channel())
    {
    }

    NIDAQZPiezo::NIDAQZPiezo( Agent *agent, Config *cfg )
      :ZPiezoBase<Config>(agent,cfg)
      ,daq(agent,"fetch_ZPiezo")
      ,_ao(cfg->channel())
    {
    }

    unsigned int NIDAQZPiezo::on_attach()
    {
      return daq.on_attach();
    }

    unsigned int NIDAQZPiezo::on_detach()
    {
      return daq.on_detach();
    }

    /*
     * Compute ZPiezo Waveform Constant:
     * ----------------------------
     *         0     N
     *         |     |
     *          ____________________ ,- z_um
     *  _______|                     ,- z previous
     */
    void NIDAQZPiezo::computeConstWaveform(float64 z_um,  float64 *data, int flyback, int n )
    {
      f64 off = z_um * _config->um2v();
      for(int i=0;i<n;++i)
        data[i] = off; // linear ramp from off to off+A
    }

    /*
     * Compute ZPiezo Waveform Ramp:
     * ----------------------------
     *
     *           0    N
     *           |    |____________  ,- z_um + z_step
     *               /
     *              /
     *             /
     *            /
     *  _________/                   ,- z_um
     *
     *  Notice that N-1 is equiv. to z_step - z_step/N
     */
    void NIDAQZPiezo::computeRampWaveform(float64 z_um, float64 step_um,  float64 *data, int flyback, int n )
    {
      f64   A = step_um * _config->um2v(),
          off = z_um * _config->um2v(),
            N = flyback;
      int i;
      for(i=0;i<flyback;++i)
        data[i] = A*(i/(N-1))+off; // linear ramp from off to off+A
      for(;i<n;++i)
        data[i] = A+off;
    }

#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )  goto_if_fail( 0==DAQWRN(expr), Error)
#define DAQTEST( expr ) (0==DAQWRN(expr))
#define DAQRTN( expr )  return_val_if_fail(0==DAQWRN(expr),1)

    int32 DAQmxMaxMinInterset(TaskHandle h, const char* name, f64 *min, f64 *max)
    { int32 ecode = DAQmxSuccess;
      f64 mn,mx;
      DAQJMP(ecode = DAQmxGetAODACRngHigh(h,name,&mx));
      DAQJMP(ecode = DAQmxGetAODACRngLow(h,name,&mn));
      *max = (*max<mx)?*max:mx; // take the min
      *min = (*min>mn)?*min:mn; // take the max
Error:
      return ecode;
    }

    int NIDAQZPiezo::moveTo(f64 z_um)     // FIXME: this is borke to all hell
    { int isok = 1;
      f64 mx  = _config->v_lim_max(),
          mn  = _config->v_lim_min();
      f64 off = z_um * _config->um2v();
      if( DAQTEST(DAQmxCreateAOVoltageChan(daq.daqtask,physicalChannel()->name(),"",
                 -0.1,0.1, //arbitrary-ish limits.  Must be different, min<max, and within device limits
                  DAQmx_Val_Volts,"")))
      {
          DAQJMP(DAQmxMaxMinInterset(daq.daqtask,physicalChannel()->name(),&mn,&mx));
          DAQJMP(DAQmxSetAOMax(daq.daqtask,physicalChannel()->name(),mx));
          DAQJMP(DAQmxSetAOMin(daq.daqtask,physicalChannel()->name(),mn));
      }
      DAQJMP(DAQmxStartTask(daq.daqtask));
      DAQJMP(DAQmxWriteAnalogScalarF64(daq.daqtask,1,10,off,NULL));

Finalize:
      if(daq.daqtask)
      { DAQJMP(DAQmxStopTask(daq.daqtask));
        //DAQJMP(DAQmxClearTask(h)); // leave this for detach
      }
      return isok;
Error:
      isok = 0;
      goto Finalize;
    }

    //
    // Simulate ZPiezo
    //

    SimulatedZPiezo::SimulatedZPiezo( Agent *agent )
      :ZPiezoBase<Config>(agent)
      ,_chan(agent,"ZPiezo")
      ,_ao("simulated")
    {}

    SimulatedZPiezo::SimulatedZPiezo( Agent *agent, Config *cfg )
      :ZPiezoBase<Config>(agent,cfg)
      ,_chan(agent,"ZPiezo")
      ,_ao("simulated")
    {}

    void SimulatedZPiezo::computeConstWaveform( float64 z_um, float64 *data, int flyback, int n )
    {
      memset(data,0,sizeof(float64)*n);
    }

    void SimulatedZPiezo::computeRampWaveform( float64 z_um, float64 step_um, float64 *data, int flyback, int n )
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
      setKind(cfg->kind());
      Guarded_Assert(_nidaq||_simulated); // at least one device was instanced
      if(_nidaq)     _nidaq->_set_config(cfg->mutable_nidaq());
      if(_simulated) _simulated->_set_config(cfg->mutable_simulated());
      _config = cfg;
    }

    void ZPiezo::_set_config( const Config &cfg )
    {
      cfg::device::ZPiezo_ZPiezoType kind = cfg.kind();
      //_config->set_kind(kind);
      _config->CopyFrom(cfg);
      Guarded_Assert(_nidaq||_simulated); // at least one device was instanced
      if(_nidaq)     _nidaq->_set_config(_config->mutable_nidaq());
      if(_simulated) _simulated->_set_config(_config->mutable_simulated());
    }

    void ZPiezo::getScanRange( f64 *min_um,f64 *max_um, f64 *step_um )
    {
      Config c = get_config();
      *min_um = c.um_min();
      *max_um = c.um_max();
      *step_um = c.um_step();
    }

  }
}
