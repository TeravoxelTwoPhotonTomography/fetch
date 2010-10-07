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
 * attach
 * detach
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
 *       NIDAQAgent\
 *                  Pockels
 *
 * NIDAQAgent implements (detach)attach methods that (de)initialize a NI-DAQmx
 * task.
 */


#ifndef POCKELS_H_
#define POCKELS_H_

#include "NIDAQAgent.h"
#include "pockels.pb.h"
#include "object.h"

#define POCKELS_DEFAULT_TIMEOUT         INFINITE
#define POCKELS_MAX_CHAN_STRING               32

namespace fetch
{

  namespace device
  {
    template<class T>
    class IPockels:public IConfigurableDevice<T>
    {
    public:
      IPockels(Agent *agent)              : IConfigurableDevice<T>(agent) {}
      IPockels(Agent *agent, Config *cfg) : IConfigurableDevice<T>(agent,cfg) {}

      virtual int is_open_val_in_bounds(f64 volts) = 0;
      virtual int set_open_val(f64 volts) = 0; 
      virtual int set_open_val__no_wait(f64 volts) = 0;

      //
      // HERE
      //
      // Trying to figure out how to do the abstract interface here
      // - How to do copy config?
      // - who should own the config after set_config?  (should be a copy, right? but now it's not)
      //


      /*
      { 
        Config cfg;
        get_config(&cfg);
        if(is_open_val_in_bounds(volts))
        { 
          cfg.set_v_open(volts);
          set_config(cfg);
        }
        else
          warning("Pockles: attempted to set v_open to an out of bounds value.\r\n");        
      }
      */
    };


    class Pockels:public IPockels<cfg::device::Pockels>
    {    
      NIDAQAgent daq;
    public:
               Pockels(Agent *agent);
               Pockels(Agent *agent, Config *cfg);

               virtual ~Pockels();

      virtual int      is_open_val_in_bounds(f64 volts);
      virtual int      set_open_val(f64 volts);
      BOOL     Set_Open_Val_Nonblocking(f64 volts);

    private:
      CRITICAL_SECTION local_state_lock;

      void __common_setup();
    };

  } // end namespace device
}   // end namespace fetch

#endif /* POCKELS_H_ */
