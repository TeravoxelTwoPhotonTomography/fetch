/*
 * ZPiezo.h
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
#pragma once
#include "NIDAQAgent.h"
#include "zpiezo.pb.h"
#include "object.h"

namespace fetch 
{ namespace device 
  {

    class IZPiezo
    {
    public:
    };

    template<class T>
    class ZPiezoBase:public IZPiezo,public IConfigurableDevice<T>
    {
    public:
      ZPiezoBase(Agent *agent) : IConfigurableDevice<T>(agent) {}
      ZPiezoBase(Agent *agent, Config *cfg) :IConfigurableDevice<T>(agent,cfg) {}
    };

    class NIDAQZPiezo:public ZPiezoBase<cfg::device::NIDAQZPiezo>      
    {
      NIDAQAgent daq;
    public:
      NIDAQZPiezo(Agent *agent);
      NIDAQZPiezo(Agent *agent, Config *cfg);

      unsigned int attach();
      unsigned int detach();
    };
      
    class SimulatedZPiezo:public ZPiezoBase<f64>
    {
    public:
      SimulatedZPiezo(Agent *agent);
      SimulatedZPiezo(Agent *agent, Config *cfg);

      unsigned int attach() {return 0;}
      unsigned int detach() {return 0;}
    };

    class ZPiezo:public ZPiezoBase<cfg::device::ZPiezo>
    {
      NIDAQZPiezo     *_nidaq;
      SimulatedZPiezo *_simulated;
      IDevice         *_idevice;
      IZPiezo         *_izpiezo;
    public:
      ZPiezo(Agent *agent);
      ZPiezo(Agent *agent, Config *cfg);
      ~ZPiezo();

      virtual unsigned int attach();
      virtual unsigned int detach();

      void setKind(Config::ZPiezoType kind);

      virtual void set_config(NIDAQZPiezo::Config *cfg);
      virtual void set_config(SimulatedZPiezo::Config *cfg);
      virtual void set_config_nowait(NIDAQZPiezo::Config *cfg);
      virtual void set_config_nowait(SimulatedZPiezo::Config *cfg);
    };

  }
}