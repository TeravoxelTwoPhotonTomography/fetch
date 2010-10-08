/*
 * Shutter.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: clackn
 */

#include "stdafx.h"
#include "shutter.h"
#include "NIDAQAgent.h"
#include "object.h"

#include "../util/util-nidaqmx.h"

#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, error  ))
#define DAQJMP( expr )   goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  namespace device
  {        

    //
    // NIDAQ Shutter
    //

    NIDAQShutter::NIDAQShutter( Agent *agent )
      :ShutterBase<cfg::device::NIDAQShutter>(agent)
      ,daq(agent,"Shutter")
    {
    }

    NIDAQShutter::NIDAQShutter( Agent *agent, Config *cfg )
      :ShutterBase<cfg::device::NIDAQShutter>(agent,cfg)
      ,daq(agent,"Shutter")
    {
    }

    unsigned int NIDAQShutter::attach()
    {
      unsigned int sts = daq.attach();
      if(sts==0) // 0 is success
        Bind();
      return sts;
    }

    unsigned int NIDAQShutter::detach()
    {
      return daq.detach();
    }


    /*
     * Note:
     * ----
     * Another way of doing this would be to follow the pattern used by the
     * Pockels class.  That is, Set() makes a call to the Task to commit the
     * new value to hardware while appropriately controlling Agent state.
     *
     * The approach used below doesn't use any state management or
     * synchronization.  As a result the implementation is simple, but there's
     * very little control over when these changes are executed.
     *
     * However, this class is used in a pretty specific context.  So far, this
     * simple implementation has been enough.
     */
    void
    NIDAQShutter::Set(u8 val)
    { int32 written = 0;
      DAQERR( DAQmxWriteDigitalLines( daq.daqtask,
                                      1,                          // samples per channel,
                                      0,                          // autostart
                                      0,                          // timeout
                                      DAQmx_Val_GroupByChannel,   // data layout,
                                      &val,                       // buffer
                                      &written,                   // (out) samples written
                                      NULL ));                    // reserved
    }

    void
    NIDAQShutter::Open(void)
    { Set(_config->open());
      Sleep( SHUTTER_DEFAULT_OPEN_DELAY_MS );  // ensures shutter fully opens before an acquisition starts
    }

    void
    NIDAQShutter::Shut(void)
    { Set(_config->closed());
    }
    
    void
    NIDAQShutter::Bind(void)
    { DAQERR( DAQmxClearTask(daq.daqtask) );
      DAQERR( DAQmxCreateTask(daq._daqtaskname,&daq.daqtask));
      DAQERR( DAQmxCreateDOChan( daq.daqtask,
                                 _config->do_channel().c_str(),
                                 "shutter-command",
                                 DAQmx_Val_ChanPerLine ));
      DAQERR( DAQmxStartTask( daq.daqtask ) );                    // go ahead and start it
      Shut();                                                    // Close the shutter....
    }

    //
    // Simulate Shutter
    //

    SimulatedShutter::SimulatedShutter( Agent *agent )
      :ShutterBase<int>(agent)
    {
      *_config=0;
    }

    SimulatedShutter::SimulatedShutter( Agent *agent, Config *cfg )
      :ShutterBase<int>(agent,cfg)
    {}

    void SimulatedShutter::Set( u8 val )
    { *_config=val;
    }

    void SimulatedShutter::Shut( void )
    { *_config=0;
    }

    void SimulatedShutter::Open( void )
    { *_config=1;
    }

    //
    // Shutter
    //

    Shutter::Shutter( Agent *agent )
      :ShutterBase<cfg::device::Shutter>(agent)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_ishutter(NULL)
    {
      setKind(_config->kind());
    }

    Shutter::Shutter( Agent *agent, Config *cfg )
      :ShutterBase<cfg::device::Shutter>(agent,cfg)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_ishutter(NULL)
    {
      setKind(cfg->kind());
    }

    Shutter::~Shutter()
    {
      if(_nidaq)     { delete _nidaq;     _nidaq=NULL; }
      if(_simulated) { delete _simulated; _simulated=NULL; }
    }

    void Shutter::setKind( Config::ShutterType kind )
    {
      switch(kind)
      {    
      case cfg::device::Shutter_ShutterType_NIDAQ:
        if(!_nidaq)
          _nidaq = new NIDAQShutter(_agent,_config->mutable_nidaq());
        _idevice  = _nidaq;
        _ishutter = _nidaq;
        break;
      case cfg::device::Shutter_ShutterType_Simulated:    
        if(!_simulated)
          _simulated = new SimulatedShutter(_agent);
        _idevice  = _simulated;
        _ishutter = _simulated;
        break;
      default:
        error("Unrecognized kind() for Shutter.  Got: %u\r\n",(unsigned)kind);
      }
    }

    void Shutter::Set( u8 v )
    {
      Guarded_Assert(_ishutter);
      _ishutter->Set(v);
    }

    void Shutter::Open()
    {
      Guarded_Assert(_ishutter);
      _ishutter->Open();
    }

    void Shutter::Shut()
    {
      Guarded_Assert(_ishutter);
      _ishutter->Shut();
    }

    void Shutter::set_config( NIDAQShutter::Config *cfg )
    {
      Guarded_Assert(_nidaq);
      _nidaq->set_config(cfg);
    }

    void Shutter::set_config_nowait( SimulatedShutter::Config *cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config_nowait(cfg);
    }

    void Shutter::set_config_nowait( NIDAQShutter::Config *cfg )
    {
      Guarded_Assert(_nidaq);
      _nidaq->set_config_nowait(cfg);
    }

    void Shutter::set_config( SimulatedShutter::Config *cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config(cfg);
    }
    unsigned int Shutter::attach()
    {
      Guarded_Assert(_idevice);
      return _idevice->attach();
    }

    unsigned int Shutter::detach()
    {
      Guarded_Assert(_idevice);
      return _idevice->detach();
    }
  }

}
