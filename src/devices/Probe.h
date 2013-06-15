/*
 * Probe.h
 *
 *  Created on: Apr 19, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#pragma once
#include "object.h"
#include "DAQChannel.h"
#include "probe.pb.h"

#define PROBE_DEFAULT_TIMEOUT           INFINITY
#define PROBE_MAX_CHAN_STRING                 32

#define PROBE_DEFAULT_OPEN_DELAY_MS           50


namespace fetch
{
  bool operator==(const cfg::device::NIDAQProbe& a, const cfg::device::NIDAQProbe& b)        ;
  bool operator==(const cfg::device::SimulatedProbe& a, const cfg::device::SimulatedProbe& b);
  bool operator==(const cfg::device::Probe& a, const cfg::device::Probe& b)                  ;
  bool operator!=(const cfg::device::NIDAQProbe& a, const cfg::device::NIDAQProbe& b)        ;
  bool operator!=(const cfg::device::SimulatedProbe& a, const cfg::device::SimulatedProbe& b);
  bool operator!=(const cfg::device::Probe& a, const cfg::device::Probe& b)                  ;


  namespace device
  {
    class IProbe
    {
    public:
      virtual float read() = 0;
      virtual void  Bind() = 0;
      virtual void  UnBind() = 0;
    };

    template<class T>
    class ProbeBase:public IProbe,public IConfigurableDevice<T>
    {
    public:
      ProbeBase(Agent *agent) : IConfigurableDevice<T>(agent) {}
      ProbeBase(Agent *agent, Config *cfg) :IConfigurableDevice<T>(agent,cfg) {}
    };

    class NIDAQProbe:public ProbeBase<cfg::device::NIDAQProbe>
    {
      NIDAQChannel daq;
      IDAQPhysicalChannel _ai;
    public:
      NIDAQProbe(Agent *agent);
      NIDAQProbe(Agent *agent, Config *cfg);
      ~NIDAQProbe();

      unsigned int on_attach();
      unsigned int on_detach();

      virtual void _set_config(Config IN *cfg) {_ai.set(cfg->ai_channel()); Bind();}

      float read();
      void  Bind (void);   // Binds the analog input channel to the daq task. - called by on_attach()
      void  UnBind(void);
    };

    class SimulatedProbe:public ProbeBase<cfg::device::SimulatedProbe>
    {
    public:
      SimulatedProbe(Agent *agent);
      SimulatedProbe(Agent *agent, Config *cfg);

      unsigned int on_attach() {return 0;}
      unsigned int on_detach() {return 0;}

      void update(void) {}
      void Bind() {}
      void UnBind() {}
      float read() {return 0.0f;}
    };

    class Probe:public ProbeBase<cfg::device::Probe>
    {
      NIDAQProbe     *_nidaq;
      SimulatedProbe *_simulated;
      IDevice        *_idevice;
      IProbe         *_iprobe;
    public:
      Probe(Agent *agent);
      Probe(Agent *agent, Config *cfg);
      ~Probe();

      virtual unsigned int on_attach() {return _idevice->on_attach();}
      virtual unsigned int on_detach() {return _idevice->on_detach();}
      void _set_config( Config IN *cfg );
      void _set_config( const Config &cfg );

      void setKind(Config::ProbeType kind);

      void  Bind() { return _iprobe->Bind();}
      void  UnBind() { return _iprobe->UnBind();}
      float read() {return _iprobe->read();}
    };

//end namespace fetch::device
  }
}
