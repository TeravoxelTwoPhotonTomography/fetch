/*
 * Scanner2D.h
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
#include "digitizer.h"
#include "pockels.h"
#include "shutter.h"
#include "LinearScanMirror.h"

//
// Device configuration
//    definitions
//    defaults
//
#define SCANNER2D_QUEUE_NUM_FRAMES                   32

#define SCANNER2D_DEFAULT_RESONANT_FREQUENCY         7920.0 // Hz
#define SCANNER2D_DEFAULT_SCANS                      512  // Number of full resonant periods that make up a frame
                                                          //        image height = 2 x scans
#define SCANNER2D_DEFAULT_LINE_DUTY_CYCLE           0.95f // Fraction of resonant period to acquire (must be less than one)
#define SCANNER2D_DEFAULT_LINE_TRIGGER_SRC              1 // Digitizer channel corresponding to resonant velocity input
                                                          // the channel should be appropriately configured in the digitizer config

#define SCANNER2D_DEFAULT_LINE_TRIGGER             "APFI0"// DAQ terminal: should be connected to resonant velocity output
#define SCANNER2D_DEFAULT_FRAME_ARMSTART           "RTSI2"// DAQ terminal: should be connected to "ReadyForStart" event output from digitizer
#define SCANNER2D_DEFAULT_DAQ_CLOCK   "Ctr1InternalOutput"// DAQ terminal: used to produce an appropriately triggered set of pulses as ao sample clock
#define SCANNER2D_DEFAULT_DAQ_CTR             "/Dev1/ctr1"// DAQ terminal: used to produce an appropriately triggered set of pulses as ao sample clock

#define SCANNER_DEFAULT_TIMEOUT               INFINITE // ms
#define SCANNER_MAX_CHAN_STRING                     32 // characters


namespace fetch
{
  namespace device
  {
    typedef struct _resonant_config
    {
      f64         frequency_Hz;
      u32         nscans;
      f32         line_duty_cycle;
      u8          line_trigger_src;
    } Resonant_Config;

    typedef struct _galvo_config
    {
      u32         nsamples;
      f64         vpp;
      f64         v_lim_max;
      f64         v_lim_min;
      char        channel [SCANNER_MAX_CHAN_STRING];
      char        trigger [SCANNER_MAX_CHAN_STRING];
      char        armstart[SCANNER_MAX_CHAN_STRING];
      char        clock   [SCANNER_MAX_CHAN_STRING];
      char        ctr     [SCANNER_MAX_CHAN_STRING];
    } Galvo_Config;

    class Scanner2D : public Digitizer, Pockels, Shutter, LinearScanMirror
    {
    public:
               Scanner2D();
      virtual ~Scanner2D();
    };

  } // end namespace device
}   // end namespace fetch
