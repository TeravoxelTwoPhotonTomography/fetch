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

      //getters
      virtual void getScanRange(f64 *min_um,f64 *max_um, f64 *step_um)=0;
      virtual f64  getMin() = 0;
      virtual f64  getMax() = 0;
      virtual f64  getStep() = 0;

      //setters      
      virtual void setMin(f64 v) = 0;
      virtual void setMax(f64 v) = 0;
      virtual void setStep(f64 v) = 0;
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

      unsigned int on_attach();
      unsigned int on_detach();

      virtual void computeConstWaveform(float64 z_um, float64 *data, int n);
      virtual void computeRampWaveform(float64 z_um, float64 *data, int n);
      
      virtual IDAQChannel* physicalChannel() {return &daq;}

      void getScanRange(f64 *min_um,f64 *max_um, f64 *step_um);

      virtual f64  getMin()       {return get_config().um_min();}
      virtual f64  getMax()       {return get_config().um_max();}
      virtual f64  getStep()      {return get_config().um_step();}
      virtual void setMin(f64 v)  {Config c = get_config(); c.set_um_min(v);  set_config_nowait(c);}
      virtual void setMax(f64 v)  {Config c = get_config(); c.set_um_max(v);  set_config_nowait(c);}
      virtual void setStep(f64 v) {Config c = get_config(); c.set_um_step(v); set_config_nowait(c);}

    };
      
    class SimulatedZPiezo:public ZPiezoBase<cfg::device::SimulatedZPiezo>
    {
      SimulatedDAQChannel _chan;
    public:
      SimulatedZPiezo(Agent *agent);
      SimulatedZPiezo(Agent *agent, Config *cfg);

      unsigned int on_attach() {return 0;}
      unsigned int on_detach() {return 0;}

      virtual void computeConstWaveform(float64 z_um, float64 *data, int n);
      virtual void computeRampWaveform(float64 z_um, float64 *data, int n);

      virtual IDAQChannel* physicalChannel() {return &_chan;}

      void getScanRange(f64 *min_um,f64 *max_um, f64 *step_um);

      virtual f64  getMin()       {return get_config().um_min();}
      virtual f64  getMax()       {return get_config().um_max();}
      virtual f64  getStep()      {return get_config().um_step();}
      virtual void setMin(f64 v)  {Config c = get_config(); c.set_um_min(v);  set_config_nowait(c);}
      virtual void setMax(f64 v)  {Config c = get_config(); c.set_um_max(v);  set_config_nowait(c);}
      virtual void setStep(f64 v) {Config c = get_config(); c.set_um_step(v); set_config_nowait(c);}
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

      virtual unsigned int on_attach() {return _idevice->on_attach();}
      virtual unsigned int on_detach() {return _idevice->on_detach();}
      void _set_config( Config IN *cfg );
      void _set_config( const Config &cfg );

      void setKind(Config::ZPiezoType kind);

      virtual void computeConstWaveform(float64 z_um, float64 *data, int n) {_izpiezo->computeConstWaveform(z_um,data,n);}
      virtual void computeRampWaveform(float64 z_um, float64 *data, int n)  {_izpiezo->computeRampWaveform(z_um,data,n);}

      virtual IDAQChannel* physicalChannel() {return _izpiezo->physicalChannel();}
      
      void getScanRange(f64 *min_um,f64 *max_um, f64 *step_um) {_izpiezo->getScanRange(min_um,max_um,step_um);}

      virtual f64  getMin()       {return _izpiezo->getMin();}
      virtual f64  getMax()       {return _izpiezo->getMax();}
      virtual f64  getStep()      {return _izpiezo->getStep();}
      virtual void setMin(f64 v)  { _izpiezo->setMin(v); }
      virtual void setMax(f64 v)  { _izpiezo->setMax(v); }
      virtual void setStep(f64 v) { _izpiezo->setStep(v);}
    };

  }
}