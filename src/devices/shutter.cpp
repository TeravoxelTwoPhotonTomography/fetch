/*
 * Shutter.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: clackn
 */

#include "common.h"
#include "shutter.h"
#include "DAQChannel.h"
#include "object.h"

#include "../util/util-nidaqmx.h"

#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )   goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{

  bool operator==(const cfg::device::NIDAQShutter& a, const cfg::device::NIDAQShutter& b)         {return equals(&a,&b);}
  bool operator==(const cfg::device::SimulatedShutter& a, const cfg::device::SimulatedShutter& b) {return equals(&a,&b);}
  bool operator==(const cfg::device::Shutter& a, const cfg::device::Shutter& b)                   {return equals(&a,&b);}
  bool operator!=(const cfg::device::NIDAQShutter& a, const cfg::device::NIDAQShutter& b)         {return !(a==b);}
  bool operator!=(const cfg::device::SimulatedShutter& a, const cfg::device::SimulatedShutter& b) {return !(a==b);}
  bool operator!=(const cfg::device::Shutter& a, const cfg::device::Shutter& b)                   {return !(a==b);}


  namespace device
  {        

    //
    // NIDAQ Shutter
    //

    NIDAQShutter::NIDAQShutter( Agent *agent )
      :ShutterBase<Config>(agent)
      ,daq(agent,"fetch_Shutter")
      ,_do(_config->do_channel())
    {
    }

    NIDAQShutter::NIDAQShutter( Agent *agent, Config *cfg )
      :ShutterBase<Config>(agent,cfg)
      ,daq(agent,"fetch_Shutter")
      ,_do(cfg->do_channel())
    {
    }

    unsigned int NIDAQShutter::on_attach()
    {
      unsigned int sts = daq.on_attach();
      if(sts==0) // 0 is success
        Bind();
      return sts;
    }

    unsigned int NIDAQShutter::on_detach()
    {
      return daq.on_detach();
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
      :ShutterBase<Config>(agent)
    {}

    SimulatedShutter::SimulatedShutter( Agent *agent, Config *cfg )
      :ShutterBase<Config>(agent,cfg)
    {}

    void SimulatedShutter::Set( u8 val )
    { 
#if 0
      Config c = get_config();
#pragma warning(push)
#pragma warning(disable:4800) // forcing to bool
      c.set_state(val);
#pragma warning(push)

      set_config(c);
#endif
    }

    void SimulatedShutter::Shut( void )
    { Set(0);
    }

    void SimulatedShutter::Open( void )
    { Set(1);
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

    void Shutter::_set_config( Config IN *cfg )
    {
      setKind(cfg->kind());
      Guarded_Assert(_nidaq||_simulated); // at least one device was instanced
      if(_nidaq)     _nidaq->_set_config(cfg->mutable_nidaq());
      if(_simulated) _simulated->_set_config(cfg->mutable_simulated());;
      _config = cfg;
    }

    void Shutter::_set_config( const Config &cfg )
    {
      cfg::device::Shutter_ShutterType kind = cfg.kind();
      _config->set_kind(kind);
      setKind(kind);
      switch(kind)
      {    
      case cfg::device::Shutter_ShutterType_NIDAQ:
        _nidaq->_set_config(const_cast<Config&>(cfg).mutable_nidaq());
        break;
      case cfg::device::Shutter_ShutterType_Simulated:    
        _simulated->_set_config(cfg.simulated());
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

//end namespace fetch::device
  }
}
