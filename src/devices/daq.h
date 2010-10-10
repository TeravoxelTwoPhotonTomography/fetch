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

namespace fetch
{

  namespace device
  {

    class IDAQ
    {
    public:      /* TODO: add interface methods*/
    };

    template<class T>
    class DAQBase:public IDAQ,public IConfigurableDevice<T>
    {
    public:
      DAQBase(Agent *agent)             :IConfigurableDevice(agent) {}
      DAQBase(Agent *agent, Config* cfg):IConfigurableDevice(agent,cfg) {}
    };

    class NationalInstrumentsDAQ : public LSMBase<cfg::device::NationalInstrumentsDAQ>
    {
    public:
      NationalInstrumentsDAQ(Agent *agent);
      NationalInstrumentsDAQ(Agent *agent, Config *cfg);

      virtual unsigned int attach() {return 0;}
      virtual unsigned int detach() {return 0;}
    };

    class SimulatedDAQ : public LSMBase<f64>
    {
    public:
      SimulatedDAQ(Agent *agent);
      SimulatedDAQ(Agent *agent, Config *cfg);

      virtual unsigned int attach() {return 0;}
      virtual unsigned int detach() {return 0;}
    };
   
   class DAQ:public LSMBase<cfg::device::DAQ>
   {
     NationalInstrumentsDAQ *_nidaq;
     SimulatedDAQ           *_simulated;
     IDevice *_idevice;
     IDAQ    *_idaq;
   public:
     DAQ(Agent *agent);
     DAQ(Agent *agent, Config *cfg);
     ~DAQ();

     void setKind(Config::DAQType kind);

     virtual void set_config(NIDAQDAQ::Config *cfg);
     virtual void set_config(SimulatedDAQ::Config *cfg);
     virtual void set_config_nowait(NIDAQDAQ::Config *cfg);
     virtual void set_config_nowait(SimulatedDAQ::Config *cfg);
   };

   //end namespace fetch::device
  }
}

