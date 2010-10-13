/*
* Digitizer.cpp
*
* Author: Nathan Clack <clackn@janelia.hhmi.org>
*   Date: Apr 20, 2010
*/
/*
* Copyright 2010 Howard Hughes Medical Institute.
* All rights reserved.
* Use is subject to Janelia Farm Research Campus Software Copyright 1.1
* license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
*/
#include "stdafx.h"
#include "../util/util-niscope.h"
#include "digitizer.h"
#include "../asynq.h"

#pragma warning(push)
#pragma warning(disable:4005)
#if 1
#define digitizer_debug(...) debug(__VA_ARGS__)
#else
#define digitizer_debug(...)
#endif
#pragma warning(pop)

// TODO: unify names. eg. change ChangeWarn to DIGWRN
#define DIGWRN( expr )  (niscope_chk( vi, expr, #expr, warning ))
#define DIGERR( expr )  (niscope_chk( vi, expr, #expr, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( vi, expr, #expr, warning ), Error)

#define CheckWarn( expression )  (niscope_chk( this->_vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( this->_vi, expression, #expression, &error   ))
#define ViErrChk( expression )    goto_if( CheckWarn(expression), Error )

namespace fetch
{
  namespace device 
  {
    NIScopeDigitizer::NIScopeDigitizer(Agent *agent)
      :DigitizerBase<cfg::device::NIScopeDigitizer>(agent)
      ,_vi(0)
    { __common_setup();
    }

    NIScopeDigitizer::NIScopeDigitizer(Agent *agent,Config *cfg)
      :DigitizerBase<cfg::device::NIScopeDigitizer>(agent,cfg)
      ,_vi(0)
    { __common_setup();
    }   

    //NIScopeDigitizer::NIScopeDigitizer(size_t nbuf, size_t nbytes_per_frame, size_t nwfm)
    //  : _vi(0)
    //{ size_t Bpp    = 2, //bytes per pixel to initially allocated for
    //         _nbuf[2] = {nbuf,nbuf},
    //           sz[2] = {nbytes_per_frame,
    //                    nwfm*sizeof(struct niScope_wfmInfo)};
    //  _alloc_qs( &this->_out, 2, _nbuf, sz );
    //}

    //Digitizer::Digitizer(const Config& cfg)
    //          : _vi(0),
    //            config(&_default_config)
    //{ config->CopyFrom(cfg);
    //  Guarded_Assert(config->IsInitialized());
    //  __common_setup();

    //} 

    NIScopeDigitizer::~NIScopeDigitizer(void)
    { if( _agent->detach() > 0 )
    warning("Could not cleanly detach. vi: %d\r\n", this->_vi);
    }

    unsigned int
      NIScopeDigitizer::detach(void)
    { ViStatus status = 1; //error

    digitizer_debug("Digitizer: Attempting to disarm. vi: %d\r\n", this->_vi);
    if(this->_vi)
      ViErrChk(niScope_close(this->_vi));  // Close the session
    status = 0;  // success
    digitizer_debug("Digitizer: Detached.\r\n");
Error:
    this->_vi = 0;
    return status;
    }

    unsigned int
      NIScopeDigitizer::attach(void)
    { ViStatus status = VI_SUCCESS;      
    digitizer_debug("Digitizer: Attach\r\n");      
    if( _vi == NULL )
    { // Open the NI-SCOPE instrument handle
      status = CheckPanic (
        niScope_init (const_cast<ViRsrc>(_config->name().c_str()),
        NISCOPE_VAL_TRUE,    // ID Query
        NISCOPE_VAL_FALSE,   // Reset?
        &_vi)                // Session
        );
    }      
    digitizer_debug("\tGot session %3d with status %d\n",_vi,status);
    return status!=VI_SUCCESS;
    }

    void NIScopeDigitizer::__common_setup()
    {
      // Setup output queues.
      // - Sizes determine initial allocations.
      // - out[0] receives raw data     from each niScope_Fetch call.
      // - out[1] receives the metadata from each niScope_Fetch call.
      //
      size_t
        Bpp = 2, //bytes per pixel to initially allocated for
        nbuf[2] = {DIGITIZER_BUFFER_NUM_FRAMES,
        DIGITIZER_BUFFER_NUM_FRAMES},
        sz[2] = { 
          _config->num_records() * _config->record_size() * _config->nchannels() * Bpp,
          _config->num_records() * sizeof(struct niScope_wfmInfo)};
          _alloc_qs( &_out, 2, nbuf, sz );
    }

    void NIScopeDigitizer::setup(int nrecords, double record_frequency_Hz, double duty)
    {
      ViSession vi =  this->NIScopeDigitizer::_vi;      

      ViReal64   refPosition         = 0.0;
      ViReal64   verticalOffset      = 0.0;
      ViReal64   probeAttenuation    = 1.0;
      ViBoolean  enforceRealtime     = NISCOPE_VAL_TRUE;

#pragma warning(push)
#pragma warning(disable:4244)
      ViInt32 record_size = duty*_config->sample_rate()/record_frequency_Hz;
#pragma warning(pop)

      Guarded_Assert(record_size>0);

      // Select the trigger channel
      if(_config->line_trigger_src() >= (unsigned) _config->channel_size())
        error("NIScopeDigitizer:\t\nTrigger source channel has not been configured.\n"
              "\tTrigger source: %hhu (out of bounds)\n"
              "\tNumber of configured channels: %d\n", 
              _config->line_trigger_src(),
              _config->channel_size());
      const Config::Channel& line_trigger_cfg = _config->channel(_config->line_trigger_src());
      Guarded_Assert( line_trigger_cfg.enabled() );

      // Configure vertical for line-trigger channel
      niscope_cfg_rtsi_default( vi );
      DIGERR( niScope_ConfigureVertical(vi,
        line_trigger_cfg.name().c_str(),  // channelName
        line_trigger_cfg.range(),
        0.0,
        line_trigger_cfg.coupling(),
        probeAttenuation,
        NISCOPE_VAL_TRUE));               // enabled

      // Configure vertical of other channels
      {
        for(int ichan=0; ichan<_config->channel_size(); ++ichan)
        { 
          if(ichan==_config->line_trigger_src())
            continue;
          const Config::Channel& c=_config->channel(ichan);
          DIGERR( niScope_ConfigureVertical(vi,
            c.name().c_str(),                    // channelName
            c.range(),
            0.0,
            c.coupling(),
            probeAttenuation,
            c.enabled() ));
        }
      }

      // Configure horizontal -
      DIGERR( niScope_ConfigureHorizontalTiming(vi,
        _config->sample_rate(),
        record_size,
        refPosition,
        nrecords,
        enforceRealtime));

      // Analog trigger for bidirectional scans
      DIGERR( niScope_ConfigureTriggerEdge (vi,
        line_trigger_cfg.name().c_str(), // channelName
        0.0,                          // triggerLevel
        NISCOPE_VAL_POSITIVE,         // triggerSlope
        line_trigger_cfg.coupling(),  // triggerCoupling
        0.0,                          // triggerHoldoff
        0.0 ));                       // triggerDelay
      // Wait for start trigger (frame sync) on PFI1
      DIGERR( niScope_SetAttributeViString( vi,
        "",
        NISCOPE_ATTR_ACQ_ARM_SOURCE,
        NISCOPE_VAL_PFI_1 ));
      return;
    }

    //
    // Alazar digitizer
    //

    void AlazarDigitizer::setup( int nrecords, double record_frequency_Hz, double duty )
    {
      error("TODO: Implement me!");
    }

    //
    // Digitizer
    //

    Digitizer::Digitizer( Agent *agent )
      :DigitizerBase<cfg::device::Digitizer>(agent)
      ,_niscope(NULL)
      ,_simulated(NULL)
      ,_alazar(NULL)
      ,_idevice(NULL)
      ,_idigitizer(NULL)
    {
      setKind(_config->kind());
    }

    Digitizer::Digitizer( Agent *agent, Config *cfg )
      :DigitizerBase<cfg::device::Digitizer>(agent,cfg)
      ,_niscope(NULL)
      ,_simulated(NULL)
      ,_alazar(NULL)
      ,_idevice(NULL)
      ,_idigitizer(NULL)
    {
      setKind(cfg->kind());
    }

    Digitizer::~Digitizer()
    {
      if(_niscope)     { delete _niscope;     _niscope=NULL; }
      if(_simulated) { delete _simulated; _simulated=NULL; }
      if(_alazar) { delete _alazar; _alazar=NULL; }
    }

    void Digitizer::setKind( Config::DigitizerType kind )
    {
      switch(kind)
      {    
      case cfg::device::Digitizer_DigitizerType_NIScope:
        if(!_niscope)
          _niscope = new NIScopeDigitizer(_agent,_config->mutable_niscope());
        _idevice  = _niscope;
        _idigitizer = _niscope;
        break;
      case cfg::device::Digitizer_DigitizerType_Alazar:
        if(!_niscope)
          _alazar = new AlazarDigitizer(_agent,_config->mutable_alazar());
        _idevice  = _niscope;
        _idigitizer = _niscope;
        break;
      case cfg::device::Digitizer_DigitizerType_Simulated:    
        if(!_simulated)
          _simulated = new SimulatedDigitizer(_agent);
        _idevice  = _simulated;
        _idigitizer = _simulated;
        break;
      default:
        error("Unrecognized kind() for Digitizer.  Got: %u\r\n",(unsigned)kind);
      }
    }

    void Digitizer::set_config(const NIScopeDigitizer::Config &cfg )
    {
      Guarded_Assert(_niscope);
      _niscope->set_config(cfg);
    }

    void Digitizer::set_config(const AlazarDigitizer::Config &cfg )
    {
      Guarded_Assert(_simulated);
      _alazar->set_config(cfg);
    }

    void Digitizer::set_config(const SimulatedDigitizer::Config &cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config(cfg);
    }

    void Digitizer::set_config_nowait(const SimulatedDigitizer::Config &cfg )
    {
      Guarded_Assert(_simulated);
      _simulated->set_config_nowait(cfg);
    }

    void Digitizer::set_config_nowait(const AlazarDigitizer::Config &cfg )
    {
      Guarded_Assert(_simulated);
      _alazar->set_config_nowait(cfg);
    }

    void Digitizer::set_config_nowait(const NIScopeDigitizer::Config &cfg )
    {
      Guarded_Assert(_niscope);
      _niscope->set_config_nowait(cfg);
    }

    unsigned int Digitizer::attach()
    {
      Guarded_Assert(_idevice);
      return _idevice->attach();
    }

    unsigned int Digitizer::detach()
    {
      Guarded_Assert(_idevice);
      return _idevice->detach();
    }

    void Digitizer::_set_config( Config IN *cfg )
    {
      _niscope->_set_config(cfg->mutable_niscope());
      _alazar->_set_config(cfg->mutable_alazar()); 
      _simulated->_set_config(cfg->mutable_simulated());;
      _config = cfg;
      setKind(cfg->kind());
    }

    void Digitizer::_set_config( const Config &cfg )
    {
      cfg::device::Digitizer_DigitizerType kind = cfg.kind();
      _config->set_kind(kind);
      setKind(kind);
      switch(kind)
      {    
      case cfg::device::Digitizer_DigitizerType_NIScope:
        _niscope->_set_config(cfg.niscope());
        break;
      case cfg::device::Digitizer_DigitizerType_Alazar:
        _alazar->_set_config(cfg.alazar());
        break;
      case cfg::device::Digitizer_DigitizerType_Simulated:    
        _simulated->_set_config(cfg.simulated());
        break;
      default:
        error("Unrecognized kind() for Digitizer.  Got: %u\r\n",(unsigned)kind);
      }
    }




  } // namespace fetch
} // namespace fetch