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
#include "common.h"
#include "util/util-niscope.h"
#include "digitizer.h"
#include "Chan.h"
#include "alazar.h"

#pragma warning(push)
#pragma warning(disable:4005)
#if 1
#define digitizer_debug(...) debug(__VA_ARGS__)
#else
#define digitizer_debug(...)
#endif
#pragma warning(pop)

// TODO: unify names. eg. change ChangeWarn to DIGWRN
#define DIGWRN( expr )  (niscope_chk( this->_vi, expr, #expr, __FILE__, __LINE__, warning ))
#define DIGERR( expr )  (niscope_chk( this->_vi, expr, #expr, __FILE__, __LINE__, error   ))
#define DIGJMP( expr )  goto_if_fail(VI_SUCCESS == niscope_chk( this->_vi, expr, #expr, __FILE__, __LINE__, warning ), Error)

#define CheckWarn( expression )  (niscope_chk( this->_vi, expression, #expression, &warning ))
#define CheckPanic( expression ) (niscope_chk( this->_vi, expression, #expression, &error   ))
#define ViErrChk( expression )    goto_if( CheckWarn(expression), Error )

#define CHKJMP(expr) \
  if(!(expr))                                                                         \
  { warning("[DIGITIZER] %s(%d)"ENDL"\tExpression: %s"ENDL"\twas false."ENDL,__FILE__,__LINE__,#expr);  \
    goto Error;                                                                       \
  }

namespace fetch
{

  bool operator==(const cfg::device::NIScopeDigitizer& a, const cfg::device::NIScopeDigitizer& b)         {return equals(&a,&b);}
  bool operator==(const cfg::device::AlazarDigitizer& a, const cfg::device::AlazarDigitizer& b)           {return equals(&a,&b);}
  bool operator==(const cfg::device::SimulatedDigitizer& a, const cfg::device::SimulatedDigitizer& b)     {return equals(&a,&b);}
  bool operator==(const cfg::device::Digitizer& a, const cfg::device::Digitizer& b)                       {return equals(&a,&b);}

  bool operator!=(const cfg::device::NIScopeDigitizer& a, const cfg::device::NIScopeDigitizer& b)         {return !(a==b);}
  bool operator!=(const cfg::device::AlazarDigitizer& a, const cfg::device::AlazarDigitizer& b)           {return !(a==b);}
  bool operator!=(const cfg::device::SimulatedDigitizer& a, const cfg::device::SimulatedDigitizer& b)     {return !(a==b);}
  bool operator!=(const cfg::device::Digitizer& a, const cfg::device::Digitizer& b)                       {return !(a==b);}

  namespace device
  {
    NIScopeDigitizer::NIScopeDigitizer(Agent *agent)
      :DigitizerBase<cfg::device::NIScopeDigitizer>(agent)
      ,_vi(0)
      ,_lastResourceName("")
    { __common_setup();
    }

    NIScopeDigitizer::NIScopeDigitizer(Agent *agent,Config *cfg)
      :DigitizerBase<cfg::device::NIScopeDigitizer>(agent,cfg)
      ,_vi(0)
      ,_lastResourceName("")
    { __common_setup();
    }

    NIScopeDigitizer::~NIScopeDigitizer(void)
    {
    }

    unsigned int
    NIScopeDigitizer::on_detach(void)
    {
#ifdef HAVE_NISCOPE
      ViStatus status = 1; //error
      digitizer_debug("Digitizer: Attempting to disarm. vi: %d\r\n", this->_vi);
      if(this->_vi)
        DIGJMP(niScope_close(this->_vi));  // Close the session
      status = 0;  // success
      digitizer_debug("Digitizer: Detached.\r\n");
      _lastResourceName.clear();
Error:
      this->_vi = 0;
      return status;
#else
      return 1; // failure - no niscope
#endif
    }

    unsigned int
      NIScopeDigitizer::on_attach(void)
    {
#ifdef HAVE_NISCOPE
      ViStatus status = VI_SUCCESS;
      digitizer_debug("NIScopeDigitizer: Attach\r\n");
      if( _vi == NULL )
      { // Open the NI-SCOPE instrument handle
        DIGJMP(
          status=niScope_init (
            const_cast<ViRsrc>(_config->name().c_str()),
            NISCOPE_VAL_TRUE,    // ID Query
            NISCOPE_VAL_FALSE,   // Reset?
            &_vi)                // Session
          );
      }
      digitizer_debug("\tGot session %3d with status %d\n",_vi,status);
      _lastResourceName = _config->name();
      return status!=VI_SUCCESS;
    Error:
#endif
      return 1; // failure
    }

    void NIScopeDigitizer::onUpdate()
    { if(_config->name().compare(_lastResourceName)!=0)
      { if(_agent->is_attached())  // resource change - need to reattach
        { int rearm = _agent->is_armed();
          Task *last = _agent->_task;
          CHKJMP(_agent->detach()==0);
          CHKJMP(_agent->attach()==0);
          if(rearm)
            CHKJMP(_agent->arm(last,_agent->_owner)==0);
        }
      }
    Error:
      return;
    }

    void NIScopeDigitizer::__common_setup()
    {
#ifdef HAVE_NISCOPE
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
#endif
    }

    unsigned NIScopeDigitizer::setup(int nrecords, double record_frequency_Hz, double duty)
    { 
#ifdef HAVE_NISCOPE
      ViSession vi = _vi;

      ViReal64   refPosition         = _config->reference();
      ViReal64   verticalOffset      = 0.0;
      ViReal64   probeAttenuation    = 1.0;
      ViBoolean  enforceRealtime     = NISCOPE_VAL_TRUE;

#pragma warning(push)
#pragma warning(disable:4244) // type conversion
      ViInt32 recsz = record_size(record_frequency_Hz,duty);
#pragma warning(pop)

      Guarded_Assert(recsz>0);

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
        0.0,                              // offset - must be 0.0 for PCI-5105
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
            verticalOffset,
            c.coupling(),
            probeAttenuation,
            c.enabled() ));
        }
      }

      // Configure horizontal -
      DIGERR( niScope_ConfigureHorizontalTiming(vi,
        _config->sample_rate(),
        recsz,
        refPosition,
        nrecords,
        enforceRealtime));

      // Analog trigger for bidirectional scans

      const Config::Trigger& trig = line_trigger_cfg.trigger();
      ViInt32 triggerSlope = (trig.slope()==Config::Trigger::RISING) ? NISCOPE_VAL_POSITIVE:NISCOPE_VAL_NEGATIVE;
      DIGERR( niScope_ConfigureTriggerEdge (vi,
        line_trigger_cfg.name().c_str(), // channelName
        trig.level_volts(),
        triggerSlope,
        line_trigger_cfg.coupling(),
        trig.holdoff_seconds(),          // time before reenable (sec)
        trig.delay_seconds() ));         // time before effect (sec) - not super sure how this will interact with the daq's frametrigger
      // Wait for start trigger (frame sync) on PFI1
      DIGERR( niScope_SetAttributeViString( vi,
        "",
        NISCOPE_ATTR_ACQ_ARM_SOURCE,
        NISCOPE_VAL_PFI_1 ));

      //// ALL DONE
      return 1;
#else 
      return 0;
#endif
    }

    size_t NIScopeDigitizer::record_size( double record_frequency_Hz, double duty )
    { size_t d;
#pragma warning(push)
#pragma warning(disable:4244) // type cast
      d=duty*_config->sample_rate()/record_frequency_Hz;
	  d=(d/256)*256;//align to 256
#pragma warning(pop)
	  return d;
    }

    bool NIScopeDigitizer::aux_info(int *n, size_t **sizes)
    {
#ifdef HAVE_NISCOPE
      *n=1;
      *sizes = (size_t*) Guarded_Malloc(sizeof(size_t),"NIScopeDigitizer::aux_info");
      (*sizes)[0] = sizeof(niScope_wfmInfo);
      return true;
#else
      return false;
#endif
    }

    //
    // Alazar digitizer
    //
#ifdef HAVE_ALAZAR
    AlazarDigitizer::AlazarDigitizer(Agent *agent)
    : DigitizerBase<cfg::device::AlazarDigitizer>(agent)
    , _ctx(0)
    {}
    AlazarDigitizer::AlazarDigitizer(Agent *agent, Config *cfg)
    : DigitizerBase<cfg::device::AlazarDigitizer>(agent,cfg)
    , _ctx(0)
    {}

    unsigned int AlazarDigitizer::on_attach() { return !alazar_attach(&_ctx); } ///< return 0 on failure and 1 on success
    unsigned int AlazarDigitizer::on_detach() { return !alazar_detach(&_ctx); } ///< return 0 on failure and 1 on success
    unsigned int AlazarDigitizer::on_disarm() { return !alazar_disarm(_ctx);}    ///< return 0 on failure and 1 on success

    unsigned AlazarDigitizer::setup( int nrecords, double record_frequency_Hz, double duty )
    { alazar_cfg_t cfg=0;
      CHKJMP(cfg=alazar_make_config());
      alazar_set_line_trigger_lvl_volts(cfg,_config->trigger_lvl_volts());
      alazar_set_image_size(cfg,record_frequency_Hz,nrecords,&duty);
      alazar_set_aux_out_board(cfg,_config->aux_out_board_id());
      //debug("[ ]Duty (aligned): %f\n",duty);
      { int i;
        for(i=0;i<_config->channels_size();++i)
        { const int iboard=_config->channels(i).board_id(),
                    ichan =_config->channels(i).chan_id();
          alazar_set_channel_enable     (cfg,iboard,ichan,_config->channels(i).enabled());
          alazar_set_channel_input_range(cfg,iboard,ichan,_config->channels(i).range());
        }
      }
      CHKJMP(alazar_arm(_ctx,cfg));
      alazar_free_config(&cfg);
      return 1;
Error:
      warning("DIGITIZER SETUP FAILED. %s(%d) %s()\n",__FILE__,__LINE__,__FUNCTION__);
      return 0;
    }
    size_t AlazarDigitizer::record_size( double record_frequency_Hz, double duty )
    {
#pragma warning(push)
#pragma warning(disable:4244) // type cast
      return duty*sample_rate()/record_frequency_Hz;
#pragma warning(pop)
    }

    int AlazarDigitizer::start()
    { CHKJMP(alazar_start(_ctx));
      return 1;
    Error:
      return 0;
    }

    int AlazarDigitizer::stop()
    { CHKJMP(alazar_stop(_ctx));
      return 1;
    Error:
      return 0;
    }

    int AlazarDigitizer::fetch(Frame* frm)
    { CHKJMP(alazar_fetch(_ctx,&frm->data,5000)); // the only reason this is well behaved is because alazar_fetch doesn't actually swap pointers.
      return 1;
Error:
      return 0;
    }

    double AlazarDigitizer::sample_rate()
    {
      switch(_config->sample_rate())
      {
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_1KSPS    : return      1000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_2KSPS    : return      2000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_5KSPS    : return      5000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_10KSPS   : return     10000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_100KSPS  : return    100000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_200KSPS  : return    200000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_500KSPS  : return    500000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_1MSPS    : return   1000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_2MSPS    : return   2000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_5MSPS    : return   5000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_10MSPS   : return  10000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_20MSPS   : return  20000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_50MSPS   : return  50000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_100MSPS  : return 100000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_125MSPS  : return 125000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_250MSPS  : return 250000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_500MSPS  : return 500000000.0; break;
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_USER_DEF : return _config->ext_clock_rate(); break; // external clock
        case cfg::device::AlazarDigitizer_SampleRate_SAMPLE_RATE_MAX: return 500000000.0; break;
      default:
        error("Did not recognize value returned by sample_rate().  Got: %d\r\n.",_config->sample_rate());
      }
      UNREACHABLE;
      return -1.0;
    }

    size_t AlazarDigitizer::nchan()
    {
      size_t n=0;
      for(int i=0;i<_config->channels().size();++i)
        if(_config->channels(i).enabled())
          ++n;
      return n;
    }

    void AlazarDigitizer::get_image_size(unsigned *w, unsigned *h)
    { alazar_get_image_size(_ctx,w,h);
    }
#else // DONT HAVE ALAZAR
    AlazarDigitizer::AlazarDigitizer(Agent *agent)
    : DigitizerBase<cfg::device::AlazarDigitizer>(agent)
    , _ctx(0)
    { error("AlazarDigitizer not supported.\n"); }
    AlazarDigitizer::AlazarDigitizer(Agent *agent, Config *cfg)
    : DigitizerBase<cfg::device::AlazarDigitizer>(agent,cfg)
    , _ctx(0)
    { error("AlazarDigitizer not supported.\n"); }
    unsigned int AlazarDigitizer::on_attach() { return 0;}
    unsigned int AlazarDigitizer::on_detach() { return 0;}
    unsigned int AlazarDigitizer::on_disarm() { return 0;}
    void AlazarDigitizer::setup( int nrecords, double record_frequency_Hz, double duty ){}
    size_t AlazarDigitizer::record_size( double record_frequency_Hz, double duty ) {return 0;}
    int AlazarDigitizer::start(){ return 0; }
    int AlazarDigitizer::stop() { return 0; }
    int AlazarDigitizer::fetch(Frame* frm) {return 0;}
    double AlazarDigitizer::sample_rate(){return 0;}
    size_t AlazarDigitizer::nchan() {return 0;}
    void AlazarDigitizer::get_image_size(unsigned *w, unsigned *h){ }
#endif // HAVE_ALAZAR
    //
    // Simulated Digitizer
    //

    size_t SimulatedDigitizer::record_size( double record_frequency_Hz, double duty )
	{ size_t d;
#pragma warning(push)
#pragma warning(disable:4244) // type cast
      d=duty*_config->sample_rate()/record_frequency_Hz;
	  d=(d/256)*256;//align to 256
#pragma warning(pop)
	  return d;
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
        if(!_alazar)
          _alazar = new AlazarDigitizer(_agent,_config->mutable_alazar());
        _idevice  = _alazar;
        _idigitizer = _alazar;
        break;
      case cfg::device::Digitizer_DigitizerType_Simulated:
        if(!_simulated)
          _simulated = new SimulatedDigitizer(_agent,_config->mutable_simulated());
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

    unsigned int Digitizer::on_attach()
    {
      Guarded_Assert(_idevice);
      return _idevice->on_attach();
    }

    unsigned int Digitizer::on_detach()
    {
      Guarded_Assert(_idevice);
      return _idevice->on_detach();
    }

    unsigned int Digitizer::on_disarm()
    {
      Guarded_Assert(_idevice);
      return _idevice->on_disarm();
    }

    void Digitizer::_set_config( Config IN *cfg )
    {
      setKind(cfg->kind()); // this will instance a device refered to in the config
      Guarded_Assert( _niscope || _alazar || _simulated );
      if(_niscope)   _niscope->_set_config(cfg->mutable_niscope());
      if(_alazar)    _alazar->_set_config(cfg->mutable_alazar());
      if(_simulated) _simulated->_set_config(cfg->mutable_simulated());;
      _config = cfg;

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
