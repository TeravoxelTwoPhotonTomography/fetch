/*
 * Shutter.h
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
#include "shutter.pb.h"

#define SHUTTER_DEFAULT_TIMEOUT           INFINITY
#define SHUTTER_MAX_CHAN_STRING                 32

#define SHUTTER_DEFAULT_OPEN_DELAY_MS           50


namespace fetch
{

  namespace device
  {
    class IShutter
    {
    public:
      virtual void Set(u8 val) = 0;
      virtual void Shut() = 0;
      virtual void Open() = 0 ;
    }; 

    template<class T>
    class ShutterBase:public IShutter,public IConfigurableDevice<T>
    {
    public:
      ShutterBase(Agent *agent) : IConfigurableDevice<T>(agent) {}
      ShutterBase(Agent *agent, Config *cfg) :IConfigurableDevice<T>(agent,cfg) {}
    };

    class NIDAQShutter:public ShutterBase<cfg::device::NIDAQShutter>      
    {
      NIDAQChannel daq;
    public:
      NIDAQShutter(Agent *agent);
      NIDAQShutter(Agent *agent, Config *cfg);

      unsigned int attach();
      unsigned int detach();

      void Set          (u8 val);
      void Shut         (void);
      void Open         (void);
      
      void Bind         (void);   // Binds the digital output channel to the daq task. - called by attach()
    };

    class SimulatedShutter:public ShutterBase<cfg::device::SimulatedShutter>
    {
    public:
      SimulatedShutter(Agent *agent);
      SimulatedShutter(Agent *agent, Config *cfg);

      unsigned int attach() {return 0;}
      unsigned int detach() {return 0;}

      void update(void) {}

      void Set          (u8 val);
      void Shut         (void);
      void Open         (void);
    };

    class Shutter:public ShutterBase<cfg::device::Shutter>
    {
      NIDAQShutter     *_nidaq;
      SimulatedShutter *_simulated;
      IDevice          *_idevice;
      IShutter         *_ishutter;
    public:
      Shutter(Agent *agent);
      Shutter(Agent *agent, Config *cfg);
      ~Shutter();

      virtual unsigned int attach() {return _idevice->attach();}
      virtual unsigned int detach() {return _idevice->detach();}
      void _set_config( Config IN *cfg );
      void _set_config( const Config &cfg );

      void setKind(Config::ShutterType kind);

      void Set          (u8 val);
      void Shut         (void);
      void Open         (void);
    };

//end namespace fetch::device
  }
}
