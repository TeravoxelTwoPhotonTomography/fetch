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

#define POCKELS_DEFAULT_TIMEOUT         INFINITY

#define POCKELS_DEFAULT_V_MAX                2.0
#define POCKELS_DEFAULT_V_MIN                0.0
#define POCKELS_DEFAULT_V_OPEN               0.0  // The value to which the pockels cell will be set during a frame.  Default should be a safe value.
#define POCKELS_DEFAULT_V_CLOSED             0.0  // The value to which the pockels cell will be set between frames.
#define POCKELS_DEFAULT_AO_CHANNEL    "/Dev1/ao2"
#define POCKELS_DEFAULT_AI_CHANNEL   "/Dev1/ai16"

#define POCKELS_CONFIG_DEFAULT \
        ({ POCKELS_DEFAULT_V_MAX,\
          POCKELS_DEFAULT_V_MIN,\
          POCKELS_DEFAULT_V_OPEN,\
          POCKELS_DEFAULT_V_CLOSED,\
          POCKELS_DEFAULT_AO_CHANNEL,\
          POCKELS_DEFAULT_AI_CHANNEL,\
        })

namespace fetch
{

  namespace device
  {

    class Pockels : public NIDAQAgent
    {
    public:
      typedef struct _pockels_config
      {
        f64         v_lim_max;
        f64         v_lim_min;
        f64         v_open;
        f64         v_closed;
        char        ao_chan[SCANNER_MAX_CHAN_STRING];
        char        ai_chan[SCANNER_MAX_CHAN_STRING]; // XXX: Unused at present
      } Config;

      Config         config;
    public:
               Pockels();
      virtual ~Pockels();

      int      Is_Volts_In_Bounds(f64 volts);
      int      Set_Open_Val(f64 volts, int time);
      BOOL     Set_Open_Val_Nonblocking(f64 volts);

    private:
      CRITICAL_SECTION local_state_lock;
    };

  } // end namespace device
}   // end namespace fetch

#endif /* POCKELS_H_ */
