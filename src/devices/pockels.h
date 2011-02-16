/*
 * Pockels.h
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
 
/* Class Pockels
 * =============
 *
 * Overview
 * --------
 * This object tracks the state associated with a Pockels cell.
 *
 * The Pockels cell itself is a variable attenuation device often used for
 * fast shuttering of an illumination laser.  As such, this object tracks
 * two voltages to emulate a two-state shutter.  This functionality is
 * implemented in a Task, but the configuration state is managed here.
 *
 * The opacity of the Pockels cell is non-linearly related to the
 * applied control voltage.  Typically, a calibration is performed so that the
 * control voltage can be looked up given a desired opacity.  Such a task
 * could operate directly on an instance of this class.
 *
 * Methods
 * -------
 * on_attach
 * on_detach
 *      These are required for usable Agent subclasses.  They create/destroy
 *      the channel specified in the configuration.
 *
 * Is_Volts_In_Bounds
 *      Performs bounds checking on input voltages.
 *
 * Set_Open_Val
 * Set_Open_Val_Nonblocking
 *      Sets the voltage to be applied for the Pockels cell "open" state.
 *      These are provided to allow control of the open voltage from other
 *      threads.  Asynchronous calls to the nonblocking routine respect the
 *      temporal order with which requests are made.
 *
 *      For changes to be committed to the armed task, the task must support
 *      the UpdateableTask interface.
 *
 * Inheritance
 * -----------
 * Agent\ (virtual)
 *       NIDAQChannel\
 *                  Pockels
 *
 * NIDAQChannel implements (on_detach)on_attach methods that (de)initialize a NI-DAQmx
 * task.
 */


#ifndef POCKELS_H_
#define POCKELS_H_

#include "DAQChannel.h"
#include "pockels.pb.h"
#include "object.h"
#include "agent.h"

#define POCKELS_DEFAULT_TIMEOUT         INFINITE
#define POCKELS_MAX_CHAN_STRING               32

namespace fetch
{

  namespace device
  {

    class IPockels
    {
    public:
      virtual int isValidOpenVolts(f64 volts) = 0;
      virtual int setOpenVolts(f64 volts) = 0; 
      virtual int setOpenVoltsNoWait(f64 volts) = 0;
      virtual f64 getOpenVolts() = 0;
      
      virtual void computeVerticalBlankWaveform(float64 *data, int n) = 0;

      virtual IDAQChannel* physicalChannel() = 0;
    };

    template<class T>
    class PockelsBase:public IPockels, public IConfigurableDevice<T>
    {
    public:
      PockelsBase(Agent *agent)              : IConfigurableDevice<T>(agent) {}
      PockelsBase(Agent *agent, Config *cfg) : IConfigurableDevice<T>(agent,cfg) {}
    };

    //
    // NIDAQPockels
    //

    class NIDAQPockels:public PockelsBase<cfg::device::NIDAQPockels>
    {    
      NIDAQChannel daq;
    public:
      NIDAQPockels(Agent *agent);
      NIDAQPockels(Agent *agent, Config *cfg);

      virtual ~NIDAQPockels();

      virtual unsigned int on_attach() {return daq.on_attach();}
      virtual unsigned int on_detach() {return daq.on_detach();}

      virtual int isValidOpenVolts(f64 volts);
      virtual int setOpenVolts(f64 volts);
      virtual int setOpenVoltsNoWait(f64 volts);
      virtual f64 getOpenVolts() {return _config->v_open();}

      virtual void computeVerticalBlankWaveform(float64 *data, int n);

      virtual IDAQChannel* physicalChannel() {return &daq;}
    };   

class SimulatedPockels:public PockelsBase<cfg::device::SimulatedPockels>
    {
      SimulatedDAQChannel _chan;
    public:
      SimulatedPockels(Agent *agent);
      SimulatedPockels(Agent *agent, Config *cfg);

      virtual unsigned int on_attach() {return 0;}
      virtual unsigned int on_detach() {return 0;}

      virtual int isValidOpenVolts(f64 volts);
      virtual int setOpenVolts(f64 volts);
      virtual int setOpenVoltsNoWait(f64 volts);
      virtual f64 getOpenVolts() {return _config->val();}

      virtual void computeVerticalBlankWaveform(float64 *data, int n);

      virtual IDAQChannel* physicalChannel() {return &_chan;}
    };
    
    class Pockels:public PockelsBase<cfg::device::Pockels>
    {
      NIDAQPockels     *_nidaq;
      SimulatedPockels *_simulated;
      IDevice          *_idevice;
      IPockels         *_ipockels;
    public:
      Pockels(Agent *agent);
      Pockels(Agent *agent, Config *cfg);
      ~Pockels();

      virtual unsigned int on_attach();
      virtual unsigned int on_detach();

      void setKind(Config::PockelsType kind);
      void _set_config( Config IN *cfg );
      void _set_config( const Config &cfg );

      virtual int isValidOpenVolts(f64 volts);
      virtual int setOpenVolts(f64 volts);
      virtual int setOpenVoltsNoWait(f64 volts);
      virtual f64 getOpenVolts() {return _ipockels->getOpenVolts();}

      virtual void computeVerticalBlankWaveform(float64 *data, int n) {_ipockels->computeVerticalBlankWaveform(data,n);}
      virtual IDAQChannel* physicalChannel() {return _ipockels->physicalChannel();}
    };

  } // end namespace device
}   // end namespace fetch

#endif /* POCKELS_H_ */
