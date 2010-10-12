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
#include "DAQChannel.h"
#include "zpiezo.pb.h"
#include "object.h"
#include "DAQChannel.h"

namespace fetch 
{ namespace device 
  {

    class IZPiezo
    {
    public:
      virtual void computeConstWaveform(float64 z_um, float64 *data, int n) = 0;
      virtual void computeRampWaveform(float64 z_um, float64 *data, int n)  = 0;
      virtual IDAQChannel* physicalChannel() = 0;
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
      NIDAQChannel daq;
    public:
      NIDAQZPiezo(Agent *agent);
      NIDAQZPiezo(Agent *agent, Config *cfg);

      unsigned int attach();
      unsigned int detach();

      virtual void computeConstWaveform(float64 z_um, float64 *data, int n);
      virtual void computeRampWaveform(float64 z_um, float64 *data, int n);
      
      virtual IDAQChannel* physicalChannel() {return &daq;}
    };
      
    class SimulatedZPiezo:public ZPiezoBase<cfg::device::SimulatedZPiezo>
    {
      SimulatedDAQChannel _chan;
    public:
      SimulatedZPiezo(Agent *agent);
      SimulatedZPiezo(Agent *agent, Config *cfg);

      unsigned int attach() {return 0;}
      unsigned int detach() {return 0;}

      virtual void computeConstWaveform(float64 z_um, float64 *data, int n);
      virtual void computeRampWaveform(float64 z_um, float64 *data, int n);

      virtual IDAQChannel* physicalChannel() {return &_chan;}
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
      void _set_config( Config IN *cfg );
      void _set_config( const Config &cfg );

      void setKind(Config::ZPiezoType kind);

      virtual void computeConstWaveform(float64 z_um, float64 *data, int n) {_izpiezo->computeConstWaveform(z_um,data,n);}
      virtual void computeRampWaveform(float64 z_um, float64 *data, int n)  {_izpiezo->computeRampWaveform(z_um,data,n);}

      virtual IDAQChannel* physicalChannel() {return _izpiezo->physicalChannel();}
    };

  }
}