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
#include "visatype.h"
#include "DAQChannel.h"

#define DAQWRN( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, warning))
#define DAQERR( expr )  (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr )  goto_if_fail( 0==DAQWRN(expr), Error)
#define DAQRTN( expr )  return_val_if_fail(0==DAQWRN(expr),1)

namespace fetch {
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

    int NationalInstrumentsDAQ::waitForDone(DWORD timeout_ms/*=INFINITE*/)
    {
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
    }

    void NationalInstrumentsDAQ::writeAO(float64 *data)
    {
      int32 written,
            N = _config->ao_samples_per_waveform();
            
#if 1
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

      DAQERR( DAQmxWriteAnalogF64(_ao.daqtask,
        N,
        0,                           // autostart?
        0.0,                         // timeout (s) - to write - 0 causes write to fail if blocked at all
        DAQmx_Val_GroupByChannel,
        data,
        &written,
        NULL));
      Guarded_Assert( written == N );
      return;
    }

    int32 NationalInstrumentsDAQ::startAO()  { DAQRTN(DAQmxStartTask(_ao.daqtask)); return 0;}
    int32 NationalInstrumentsDAQ::startCLK() { DAQRTN(DAQmxStartTask(_clk.daqtask)); return 0;}
    int32 NationalInstrumentsDAQ::stopAO()   { DAQRTN(DAQmxStopTask(_ao.daqtask)); return 0;}
    int32 NationalInstrumentsDAQ::stopCLK()  { DAQRTN(DAQmxStopTask(_clk.daqtask)); return 0;}

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
      TaskHandle cur_task = 0;

      ViInt32    N = _config->ao_samples_per_waveform();
      float64 freq = computeSampleFrequency(nrecords, record_frequency_Hz);

      // The "fake" initialization
      DAQERR( DAQmxClearTask(_clk.daqtask) ); // Once a DAQ task is started, it needs to be cleared before restarting

      DAQERR( DAQmxCreateTask("fetch_CLK",&_clk.daqtask));
      cur_task = _clk.daqtask;
      DAQERR( DAQmxCreateCOPulseChanFreq(cur_task,
              _config->ctr_alt().c_str(),     // "Dev1/ctr0"
              "CLK",
              DAQmx_Val_Hz,
              DAQmx_Val_Low,
              0.0,
              freq,
              0.5 ));
      DAQERR( DAQmxStartTask(cur_task) );

      // The "real" initialization      
      DAQERR( DAQmxClearTask(_clk.daqtask) ); // Once a DAQ task is started, it needs to be cleared before restarting
      DAQERR( DAQmxCreateTask("fetch_CLK",&_clk.daqtask));      
      cur_task = _clk.daqtask;
      DAQERR( DAQmxCreateCOPulseChanFreq(cur_task,       // task
              _config->ctr().c_str(),     // "Dev1/ctr1"
              "CLK",          // name
              DAQmx_Val_Hz,   // units
              DAQmx_Val_Low,  // idle state - resting state of the output terminal
              0.0,            // delay before first pulse
              freq,           // the frequency at which to generate pulse
              0.5 ));         // duty cycle

      DAQERR( DAQmxCfgImplicitTiming           ( cur_task, DAQmx_Val_FiniteSamps, N ));
      DAQERR( DAQmxCfgDigEdgeStartTrig         ( cur_task, "AnalogComparisonEvent", DAQmx_Val_Rising ));
      DAQERR( DAQmxSetArmStartTrigType         ( cur_task, DAQmx_Val_DigEdge ));
      DAQERR( DAQmxSetDigEdgeArmStartTrigSrc   ( cur_task, _config->armstart().c_str() ));
      DAQERR( DAQmxSetDigEdgeArmStartTrigEdge  ( cur_task, DAQmx_Val_Rising ));
    }

    void NationalInstrumentsDAQ::setupAO( float64 nrecords, float64 record_frequency_Hz )
    {
      
      float64 freq = computeSampleFrequency(nrecords, record_frequency_Hz);

      DAQERR( DAQmxClearTask(_ao.daqtask) );
      DAQERR( DAQmxCreateTask( "fetch_AO", &_ao.daqtask));
      registerDoneEvent();
    }

    #define MAX_CHAN_STRING 1024    
    void NationalInstrumentsDAQ::setupAOChannels( float64 nrecords, float64 record_frequency_Hz, float64 vmin, float64 vmax, IDAQPhysicalChannel **channels, int nchannels )
    {
      char aochan[MAX_CHAN_STRING];
      float64 freq = computeSampleFrequency(nrecords,record_frequency_Hz);
      // concatenate the physical channel names
      memset(aochan, 0, sizeof(aochan));
      {
        int ichan;
        for(ichan=0;ichan<nchannels-1;++ichan)
        {
          strcat(aochan,channels[ichan]->name());
          strcat(aochan, ",");
        }
        strcat(aochan,channels[ichan]->name());
      }

      //
      {
        f64 v[4];
        // NI DAQ's typically have multiple voltage ranges capable of achieving different precisions.
        // The 6259 has 2 ranges.
        DAQERR(DAQmxGetDevAOVoltageRngs(_config->name().c_str(),v,4));
        vmin = MAX(vmin,v[2]);        
        vmax = MIN(vmax,v[3]);        
      } 

      TaskHandle cur_task = _ao.daqtask;
      DAQERR( DAQmxCreateAOVoltageChan(cur_task,
        aochan,                                  //eg: "/Dev1/ao0,/Dev1/ao2,/Dev1/ao1"
        "AO Channels",                           //name to assign to channel
        vmin,                                    //Volts eg: -10.0
        vmax,                                    //Volts eg:  10.0
        DAQmx_Val_Volts,                         //Units
        NULL));                                  //Custom scale (none)
      DAQERR( DAQmxSetWriteRegenMode(cur_task,DAQmx_Val_DoNotAllowRegen));

      DAQERR( DAQmxCfgAnlgEdgeStartTrig(cur_task,
        _config->trigger().c_str(),        
        DAQmx_Val_Rising,
        0.0));

      DAQERR( DAQmxCfgSampClkTiming(cur_task,
        _config->clock().c_str(),// eg. "Ctr1InternalOutput"        
        freq,
        DAQmx_Val_Rising,
        DAQmx_Val_ContSamps, // use continuous output so that counter stays in control
        _config->ao_samples_per_waveform()));
    }

    float64 NationalInstrumentsDAQ::computeSampleFrequency( float64 nrecords, float64 record_frequency_Hz )
    {
      ViInt32   N          = _config->ao_samples_per_waveform();
      float64   frame_time = nrecords/record_frequency_Hz;         //  512 records / (7920 records/sec)
      return N/frame_time;                         // 4096 samples / 64 ms = 63 kS/s
    }

    void NationalInstrumentsDAQ::__common_setup()
    {
      Guarded_Assert_WinErr(_notify_done=CreateEvent(NULL,FALSE,FALSE,NULL)); // auto-reset, initially untriggered
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

