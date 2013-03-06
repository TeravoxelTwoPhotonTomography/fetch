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

#include "common.h"
#include "daq.h"
#include "DAQChannel.h"

#define countof(e) (sizeof(e)/sizeof(*e))

#define REPORT(estr)    warning("%s(%d): %s()\n\tExpression evaluated as false.\n\t%s\n",__FILE__,__LINE__,__FUNCTION__,estr)
#define TRY(e)          do{if(!(e)) {REPORT(#e); goto Error;}}while(0)

#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )  goto_if_fail( 0==DAQWRN(expr), Error)
#define DAQRTN( expr )  return_val_if_fail(0==DAQWRN(expr),1)

void silent(const char* fmt, ...){}
#define DAQSILENT( expr )     (Guarded_DAQmx__Silent_Warnings( (expr), #expr, __FILE__, __LINE__, silent))
#define DAQRTN_SILENT( expr )  return_val_if_fail(0==DAQSILENT(expr),1)
#define DAQJMP_SILENT( expr )  goto_if_fail( 0==DAQSILENT(expr), Error)



namespace fetch {

  bool operator==(const cfg::device::NationalInstrumentsDAQ& a, const cfg::device::NationalInstrumentsDAQ& b)  {return equals(&a,&b);}
  bool operator==(const cfg::device::SimulatedDAQ& a, const cfg::device::SimulatedDAQ& b)                      {return equals(&a,&b);}
  bool operator==(const cfg::device::DAQ& a, const cfg::device::DAQ& b)                                        {return equals(&a,&b);}
  bool operator!=(const cfg::device::NationalInstrumentsDAQ& a, const cfg::device::NationalInstrumentsDAQ& b)  {return !(a==b);}
  bool operator!=(const cfg::device::SimulatedDAQ& a, const cfg::device::SimulatedDAQ& b)                      {return !(a==b);}
  bool operator!=(const cfg::device::DAQ& a, const cfg::device::DAQ& b)                                        {return !(a==b);}


  namespace device {

    //
    // NationalInstrumentsDAQ
    //

    NationalInstrumentsDAQ::NationalInstrumentsDAQ(Agent *agent)
      :DAQBase<Config>(agent)
      ,_clk(agent,"fetch_CLK")
      ,_ao(agent,"fetch_AO")
    {
      __common_setup();
    }

    NationalInstrumentsDAQ::NationalInstrumentsDAQ(Agent *agent,Config *cfg)
      :DAQBase<Config>(agent,cfg)
      ,_clk(agent,"fetch_CLK")
      ,_ao(agent,"fetch_AO")
    {
      __common_setup();
    }

    NationalInstrumentsDAQ::~NationalInstrumentsDAQ()
    {
      Guarded_Assert_WinErr__NoPanic(CloseHandle(_notify_done));
    }

    int32 CVICALLBACK _daq_event_done_callback(TaskHandle taskHandle, int32 sts, void *callbackData)
    {
      NationalInstrumentsDAQ *self = (NationalInstrumentsDAQ*)(callbackData);
      DAQERR(sts);
      Guarded_Assert_WinErr(SetEvent(self->_notify_done));
      return 0;
    }

    void NationalInstrumentsDAQ::registerDoneEvent( void )
    {
      DAQERR( DAQmxRegisterDoneEvent (
        _clk.daqtask,                      //task handle
        0,                                 //run the callback in a DAQmx thread
        &_daq_event_done_callback,         //callback
        (void*)this));                     //data passed to callback
    }

    unsigned int NationalInstrumentsDAQ::on_attach()
    {
      int sts = 0;
      sts  = _clk.on_attach();
      sts |= _ao.on_attach();
      return sts;
    }

    unsigned int NationalInstrumentsDAQ::on_detach()
    {
      int sts = 0;
      sts  = _ao.on_detach();
      sts |= _clk.on_detach();
      return sts;
    }

    void NationalInstrumentsDAQ::onUpdate()
    { _ao.onUpdate();
      _clk.onUpdate();
    }

    int NationalInstrumentsDAQ::waitForDone(DWORD timeout_ms/*=INFINITE*/)
    {
#if 0
      HANDLE hs[] = {
        _notify_done,
        _agent->_notify_stop};
      DWORD  res;
      const char classname[] = "NationalInstrumentsDAQ";
      res = WaitForMultipleObjects(2,hs,FALSE,timeout_ms);
      if(res == WAIT_TIMEOUT)
        error("%s: Timed out waiting for DAQ to finish AO write.\r\n",classname);
      else if(res==WAIT_ABANDONED_0)
      {
        warning("%s: Abandoned wait on notify_daq_done.\r\n",classname);
        return 1;
      }
      else if(res==WAIT_ABANDONED_0+1)
      {
        warning("%s: Abandoned wait on notify_stop.\r\n",classname);
        return 1;
      }
      return 0;
#endif
      DAQJMP_SILENT(DAQmxWaitUntilTaskDone(_ao.daqtask,timeout_ms/1000));
      return 1;
    Error:
      return 0;
    }

    int NationalInstrumentsDAQ::writeAO(float64 *data)
    {
      int32 written,
            N = _config->ao_samples_per_waveform();

#if 0
      {
        uInt32 nchan;
        char
          buf[1024],
          name[1024];
        memset(buf ,0,sizeof(buf ));
        memset(name,0,sizeof(name));
        DAQWRN( DAQmxGetTaskName(_ao.daqtask,name,sizeof(name)) );
        DAQWRN( DAQmxGetTaskNumChans(_ao.daqtask,&nchan) );
        DAQWRN( DAQmxGetTaskChannels(_ao.daqtask,buf,sizeof(buf)) );
        HERE;
        debug("NI-DAQmx Task (%s) has %d channels"ENDL"\t%s"ENDL,
          name,
          nchan,
          buf);

        while(nchan--)
        { DAQWRN( DAQmxGetNthTaskChannel(_ao.daqtask, 1+nchan, buf, sizeof(buf)) );
          DAQWRN( DAQmxGetPhysicalChanName(_ao.daqtask, buf, buf, sizeof(buf)) );
          debug("\t%d:\t%s"ENDL,nchan,buf);
        }
      }
      {
        FILE *fp = fopen("NationalInstrumentsDAQ_writeAO.f64","wb");
        fwrite(data,sizeof(f64),3*N,fp);
        fclose(fp);
      }
#endif

      DAQJMP( DAQmxWriteAnalogF64(_ao.daqtask,
        N,
        0,                           // autostart?
        1.0,                         // timeout (s) - to write - 0 causes write to fail if blocked at all
        DAQmx_Val_GroupByChannel,
        data,
        &written,
        NULL));
      Guarded_Assert( written == N );
      return 0; // success
Error:
      return 1; // fail
    }

    int32 NationalInstrumentsDAQ::startAO()  { DAQRTN(DAQmxStartTask(_ao.daqtask)); return 0;}
    int32 NationalInstrumentsDAQ::startCLK() { DAQRTN(DAQmxStartTask(_clk.daqtask)); return 0;}
    int32 NationalInstrumentsDAQ::stopAO()   { DAQRTN_SILENT(DAQmxStopTask(_ao.daqtask)); return 0;}
    int32 NationalInstrumentsDAQ::stopCLK()  { DAQRTN_SILENT(DAQmxStopTask(_clk.daqtask)); return 0;}

    /** Renders the full terminal string in buf.
        Example:
        "ctr1" for "Dev1" -> "/Dev1/ctr1"
        \returns buf on success, otherwise returns 0.
    */
    static char* make_terminal_name(char *buf, size_t nbuf, const char* dev, const char* term)
    { memset(buf,0,nbuf);
      snprintf(buf,nbuf,"/%s/%s",dev,term);
      return buf;
    }

    //************************************
    // Method:    setupCLK
    // FullName:  fetch::device::NationalInstrumentsDAQ::setupCLK
    // Access:    public
    // Returns:   void
    // Qualifier:
    //
    // set up counter for sample clock
    // - A finite pulse sequence is generated by a pair of on-board counters.
    //   In testing, it appears that after the device is reset, initializing
    //   the counter task doesn't work quite right.  First, I have to start the
    //   task with the paired counter once.  Then, I can set things up normally.
    //   After initializing with the paired counter once, things work fine until
    //   the device (or computer) is reset.  My guess is this is a fault of the
    //   board or driver software.
    // - below, we just cycle the counters when config gets called.  This ensures
    //   everything configures correctly the first time, even after a device
    //   reset or cold start.
    //************************************
    void NationalInstrumentsDAQ::setupCLK(float64 nrecords, float64 record_frequency_Hz)
    {
      TaskHandle clk = 0;

      int32      N = _config->ao_samples_per_waveform();
      float64   hz = computeSampleFrequency(nrecords, record_frequency_Hz);
      const char *dev          =_config->name().c_str(),
                 *ctr          =_config->ctr().c_str(),
                 *armstart_in  =_config->armstart().c_str(),
                 *gate_out     =_config->frame_trigger_out().c_str(),
                 *trig         =_config->trigger().c_str();
      float64     lvl          =_config->level_volts();
      char term_ctr[1024]={0},
           term_gate[1024]={0},
           term_arm_out[1024]={0};
      make_terminal_name(term_ctr    ,sizeof(term_ctr)    ,dev,ctr);
      make_terminal_name(term_gate   ,sizeof(term_gate)   ,dev,ctr);
      strcat(term_gate,"Gate");
      make_terminal_name(term_arm_out,sizeof(term_arm_out),dev,gate_out);

      DAQERR( DAQmxClearTask(_clk.daqtask) ); // Once a DAQ task is started, it needs to be cleared before restarting
      DAQERR( DAQmxCreateTask("fetch_CLK",&_clk.daqtask));
      clk = _clk.daqtask;
      DAQERR(DAQmxCreateCOPulseChanFreq     (clk,term_ctr,NULL,DAQmx_Val_Hz,DAQmx_Val_Low,0.0,hz,0.5));
      DAQERR(DAQmxCfgImplicitTiming         (clk,DAQmx_Val_FiniteSamps,N));
      DAQERR(DAQmxCfgDigEdgeStartTrig       (clk,"AnalogComparisonEvent",DAQmx_Val_Rising));
      DAQERR(DAQmxSetArmStartTrigType       (clk,DAQmx_Val_DigEdge));             // Arm trigger has to be through clk
      DAQERR(DAQmxSetDigEdgeArmStartTrigSrc (clk,armstart_in));
      DAQERR(DAQmxSetDigEdgeArmStartTrigEdge(clk,DAQmx_Val_Rising));
      DAQERR(DAQmxSetStartTrigRetriggerable (clk,1));
      DAQERR(DAQmxConnectTerms(term_gate,term_arm_out,DAQmx_Val_DoNotInvertPolarity));
    }

    void NationalInstrumentsDAQ::setupAO( float64 nrecords, float64 record_frequency_Hz )
    {

      float64 freq = computeSampleFrequency(nrecords, record_frequency_Hz);

      DAQERR( DAQmxClearTask(_ao.daqtask) );
      DAQERR( DAQmxCreateTask( "fetch_AO", &_ao.daqtask));
      registerDoneEvent();
    }


    /** Renders a full terminal list using make_terminal_name().
        \a buf should be a preallocated and zero'd bunch of bytes.  The string will
        be rendered to \a buf.
        The algorithm is not efficient and there is no bounds checking.
        \returns \a buf on success, otherwise 0.
    */
    static char *cat_terminal_names(char *buf, size_t nbuf, const char* dev, const char **terminals, size_t nterm)
    { char tmp[1024]={0};
      size_t i;
      TRY(nterm && dev && terminals);
      strcat(buf,make_terminal_name(tmp,sizeof(tmp),dev,terminals[0]));
      for(i=1;i<nterm;++i)
      { strcat(buf,",");
        strcat(buf,make_terminal_name(tmp,sizeof(tmp),dev,terminals[i]));
      }
      return buf;
    Error:
      return 0;
    }

    #define MAX_CHAN_STRING 1024
    void NationalInstrumentsDAQ::setupAOChannels( float64 nrecords,
                                                  float64 record_frequency_Hz,
                                                  float64 vmin,
                                                  float64 vmax,
                                                  IDAQPhysicalChannel **channels,
                                                   int nchannels )
    { TaskHandle  ao                    = 0;
      int32       N                     = _config->ao_samples_per_waveform();
      char        terms[MAX_CHAN_STRING]= {0};
      const char *dev                   = _config->name().c_str(),
                 *trig                  = _config->trigger().c_str();
      char        clk[MAX_CHAN_STRING]  = {0};
      float64     hz                    = computeSampleFrequency(nrecords,record_frequency_Hz),
                  lvl                   = _config->level_volts();

      strcat(clk,_config->ctr().c_str());
      strcat(clk,"InternalOutput");

      // terminal names
      TRY(nchannels<countof(terms));
      { const char* names[8]={0};
        for(int i=0;i<nchannels;++i)
          names[i]=channels[i]->name();
        cat_terminal_names(terms,sizeof(terms),dev,names,nchannels);
      }

      // voltage range
      { f64 v[4];
        DAQERR(DAQmxGetDevAOVoltageRngs(_config->name().c_str(),v,4));
        vmin = MAX(vmin,v[2]);
        vmax = MIN(vmax,v[3]);
      }

      ao=_ao.daqtask;
      DAQERR(DAQmxCreateAOVoltageChan (ao,terms,NULL,vmin,vmax,DAQmx_Val_Volts,NULL));
      DAQERR(DAQmxCfgSampClkTiming    (ao,clk,hz,DAQmx_Val_Rising,DAQmx_Val_ContSamps,N));
      DAQERR(DAQmxCfgOutputBuffer     (ao,10*N));
      DAQERR(DAQmxSetWriteRegenMode   (ao,DAQmx_Val_DoNotAllowRegen));
      DAQERR(DAQmxSetWriteRelativeTo  (ao,DAQmx_Val_CurrWritePos));
      DAQERR(DAQmxSetAODataXferMech   (ao,terms,DAQmx_Val_DMA));
      DAQERR(DAQmxSetAODataXferReqCond(ao,terms,DAQmx_Val_OnBrdMemNotFull));
      DAQERR(DAQmxCfgAnlgEdgeStartTrig(ao,trig,DAQmx_Val_Rising,lvl));
      return;
Error:
      UNREACHABLE;
    }

    float64 NationalInstrumentsDAQ::computeSampleFrequency( float64 nrecords, float64 record_frequency_Hz )
    {
      int32     N          = _config->ao_samples_per_waveform();
      float64   frame_time = (nrecords+_config->flyback_scans())/record_frequency_Hz;  //  (256+16 records) / (7920 records/sec)
      return N/frame_time;                         // 16384 samples / 32 ms = 512 kS/s
    }

    void NationalInstrumentsDAQ::__common_setup()
    {
      Guarded_Assert_WinErr(_notify_done=CreateEvent(NULL,FALSE,FALSE,NULL)); // auto-reset, initially untriggered
    }

    int NationalInstrumentsDAQ::writeOneToAO(float64 *data)
    { TaskHandle ao=_ao.daqtask;
      int32 written;
      int32 type;
      char terms[MAX_CHAN_STRING]={0};

      //DAQJMP(DAQmxGetSampTimingType(ao,&type));
      //DAQJMP(DAQmxSetSampTimingType(ao,DAQmx_Val_OnDemand));
      DAQJMP(DAQmxWriteAnalogF64(ao,1,1,0.0,DAQmx_Val_GroupByChannel,data,&written,NULL));
      //DAQJMP(DAQmxSetSampTimingType(ao,type));
      return 1;
    Error:
      return 0;
    }

    //
    // SimulatedDAQ
    //

    SimulatedDAQ::SimulatedDAQ( Agent *agent )
      :DAQBase<Config>(agent)
    {}

    SimulatedDAQ::SimulatedDAQ( Agent *agent, Config *cfg )
      :DAQBase<Config>(agent,cfg)
    {}

    //
    // DAQ
    //

    DAQ::DAQ( Agent *agent )
      :DAQBase<Config>(agent)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_idaq(NULL)
    {
       setKind(_config->kind());
    }

    DAQ::DAQ( Agent *agent, Config *cfg )
      :DAQBase<Config>(agent,cfg)
      ,_nidaq(NULL)
      ,_simulated(NULL)
      ,_idevice(NULL)
      ,_idaq(NULL)
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
        _idaq = _nidaq;
        break;
      case cfg::device::DAQ_DAQKind_Simulated:
        if(!_simulated)
          _simulated = new SimulatedDAQ(_agent);
        _idevice  = _simulated;
        _idaq = _simulated;
        break;
      default:
        error("Unrecognized kind() for DAQ.  Got: %u\r\n",(unsigned)kind);
      }
    }

    void DAQ::_set_config( Config IN *cfg )
    {
      setKind(cfg->kind());
      Guarded_Assert(_nidaq||_simulated); // at least one device was instanced
      if(_nidaq)     _nidaq->_set_config(cfg->mutable_nidaq());
      if(_simulated) _simulated->_set_config(cfg->mutable_simulated());
      _config = cfg;
    }

    void DAQ::_set_config( const Config &cfg )
    {
      cfg::device::DAQ_DAQKind kind = cfg.kind();
      _config->set_kind(kind);
      setKind(kind);
      switch(kind)
      {
      case cfg::device::DAQ_DAQKind_NIDAQ:
        _nidaq->_set_config(cfg.nidaq());
        break;
      case cfg::device::DAQ_DAQKind_Simulated:
        _simulated->_set_config(cfg.simulated());
        break;
      default:
        error("Unrecognized kind() for DAQ.  Got: %u\r\n",(unsigned)kind);
      }
    }

    //end namespace fetch::device
  }
}

