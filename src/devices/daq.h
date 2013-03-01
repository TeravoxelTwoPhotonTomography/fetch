/*
 * DAQ.h
 *
 *  Created on: Oct 10, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
 /*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#pragma once
#include "agent.h"
#include "daq.pb.h"
#include "object.h"
#include "DAQChannel.h"

void __common_setup();

namespace fetch
{

  bool operator==(const cfg::device::NationalInstrumentsDAQ& a, const cfg::device::NationalInstrumentsDAQ& b);
  bool operator==(const cfg::device::SimulatedDAQ& a, const cfg::device::SimulatedDAQ& b);
  bool operator==(const cfg::device::DAQ& a, const cfg::device::DAQ& b);
  bool operator!=(const cfg::device::NationalInstrumentsDAQ& a, const cfg::device::NationalInstrumentsDAQ& b);
  bool operator!=(const cfg::device::SimulatedDAQ& a, const cfg::device::SimulatedDAQ& b);
  bool operator!=(const cfg::device::DAQ& a, const cfg::device::DAQ& b);

  namespace device
  {

    class IDAQ
    {
    public:

      virtual int waitForDone(DWORD timeout_ms=INFINITE) = 0;                    ///<\returns 1 on fail, 0 on success

      virtual void setupCLK(float64 nrecords, float64 record_frequency_Hz) = 0;
      virtual void setupAO(float64 nrecords, float64 record_frequency_Hz) = 0;
      virtual void setupAOChannels(float64 nrecords,
                           float64 record_frequency_Hz,
                           float64 vmin,
                           float64 vmax,
                           IDAQPhysicalChannel **channels,
                           int nchannels) = 0;

      virtual int writeAO(float64 *data) = 0;                                    ///<\returns 1 on fail, 0 on success
      virtual int writeOneToAO(float64 *data)=0;

      //These should return 1 on fail, 0 on success
      virtual int32 startAO() = 0;
      virtual int32 startCLK() = 0;
      virtual int32 stopAO() = 0;
      virtual int32 stopCLK() = 0;

      virtual float64 computeSampleFrequency( float64 nrecords, float64 record_frequency_Hz ) = 0;
      virtual int32   samplesPerRecordAO() = 0;
      virtual int32   flybackSampleIndex(int nscans)=0;
    };

    template<class T>
    class DAQBase:public IDAQ,public IConfigurableDevice<T>
    {
    public:
      DAQBase(Agent *agent)             :IConfigurableDevice(agent) {}
      DAQBase(Agent *agent, Config* cfg):IConfigurableDevice(agent,cfg) {}
    };

    class NationalInstrumentsDAQ : public DAQBase<cfg::device::NationalInstrumentsDAQ>
    {
      NIDAQChannel _clk,_ao;
    public:
      NationalInstrumentsDAQ(Agent *agent);
      NationalInstrumentsDAQ(Agent *agent, Config *cfg);
      ~NationalInstrumentsDAQ();

      virtual unsigned int on_attach();
      virtual unsigned int on_detach();

      virtual void onUpdate();

      int waitForDone(DWORD timeout_ms=INFINITE);

      void setupCLK(float64 nrecords, float64 record_frequency_Hz);
      void setupAO(float64 nrecords, float64 record_frequency_Hz);
      void setupAOChannels(float64 nrecords, float64 record_frequency_Hz, float64 vmin, float64 vmax, IDAQPhysicalChannel **channels, int nchannels);

      int writeAO(float64 *data);
      int writeOneToAO(float64 *data);

      int32 startAO();
      int32 startCLK();
      int32 stopAO();
      int32 stopCLK();

      float64 computeSampleFrequency( float64 nrecords, float64 record_frequency_Hz );
      int32   samplesPerRecordAO() {return _config->ao_samples_per_waveform();}
      int32   flybackSampleIndex(int nscans) {return (samplesPerRecordAO()*nscans)/(_config->flyback_scans()+nscans);}
    protected:
      void registerDoneEvent(void);
    private:
      void __common_setup();
    public:
      HANDLE _notify_done;
    };

    class SimulatedDAQ : public DAQBase<cfg::device::SimulatedDAQ>
    {
    public:
      SimulatedDAQ(Agent *agent);
      SimulatedDAQ(Agent *agent, Config *cfg);

      virtual unsigned int on_attach() {return 0;}
      virtual unsigned int on_detach() {return 0;}

      int waitForDone(DWORD timeout_ms=INFINITE) {return 0;}

      void setupCLK(float64 nrecords, float64 record_frequency_Hz) {}
      void setupAO(float64 nrecords, float64 record_frequency_Hz)  {}
      void setupAOChannels(float64 nrecords, float64 record_frequency_Hz, float64 vmin, float64 vmax, IDAQPhysicalChannel **channels, int nchannels) {}

      int writeAO(float64 *data) {return 0;}
      int writeOneToAO(float64 *data) {return 1;};

      int32 startAO()  { return 0;}
      int32 startCLK() { return 0;}
      int32 stopAO()  { return 0;}
      int32 stopCLK() { return 0;}

      float64 computeSampleFrequency( float64 nrecords, float64 record_frequency_Hz ) {return _config->sample_frequency_hz();}
      int32   samplesPerRecordAO() {return _config->samples_per_record();}
      int32   flybackSampleIndex(int nscans) {return _config->samples_per_record()*0.95;}
    };

   class DAQ:public DAQBase<cfg::device::DAQ>
   {
     NationalInstrumentsDAQ *_nidaq;
     SimulatedDAQ           *_simulated;
     IDevice *_idevice;
     IDAQ    *_idaq;
   public:
     DAQ(Agent *agent);
     DAQ(Agent *agent, Config *cfg);
     ~DAQ();

     void setKind(Config::DAQKind kind);
     void _set_config( Config IN *cfg );
     void _set_config( const Config &cfg );

     virtual unsigned int on_attach() {return _idevice->on_attach();}
     virtual unsigned int on_detach() {return _idevice->on_detach();}

     int waitForDone(DWORD timeout_ms=INFINITE) {return _idaq->waitForDone(timeout_ms);}

     void setupCLK(float64 nrecords, float64 record_frequency_Hz) {_idaq->setupCLK(nrecords,record_frequency_Hz);}
     void setupAO(float64 nrecords, float64 record_frequency_Hz)  {_idaq->setupAO (nrecords,record_frequency_Hz);}
     void setupAOChannels(float64 nrecords, float64 record_frequency_Hz, float64 vmin, float64 vmax, IDAQPhysicalChannel **channels, int nchannels)
     {_idaq->setupAOChannels(nrecords,record_frequency_Hz,vmin,vmax,channels,nchannels);}

     int writeAO(float64 *data) {return _idaq->writeAO(data);}
     int writeOneToAO(float64 *data) {return _idaq->writeOneToAO(data);}

     int32 startAO()  { return _idaq->startAO();}
     int32 startCLK() { return _idaq->startCLK();}
     int32 stopAO()   { return _idaq->stopAO();}
     int32 stopCLK()  { return _idaq->stopCLK();}

     float64 computeSampleFrequency( float64 nrecords, float64 record_frequency_Hz ) {return _idaq->computeSampleFrequency(nrecords,record_frequency_Hz);};
     int32   samplesPerRecordAO() {return _idaq->samplesPerRecordAO();}
     int32   flybackSampleIndex(int nscans) {return _idaq->flybackSampleIndex(nscans);}
   };

   //end namespace fetch::device
  }
}

