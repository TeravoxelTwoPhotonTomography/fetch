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
#include "NIDAQAgent.h"
#include "shutter.pb.h"

#define SHUTTER_DEFAULT_TIMEOUT           INFINITY
#define SHUTTER_MAX_CHAN_STRING                 32

#define SHUTTER_DEFAULT_OPEN_DELAY_MS           50


namespace fetch
{

  namespace device
  {

    class Shutter : 
      public NIDAQAgent, 
      public Configurable<cfg::device::Shutter>
    {
    public:
      Shutter();
      Shutter(Config *cfg);

      void Set          (u8 val);
      void Close        (void);
      void Open         (void);
      
      void Bind         (void);   // Binds the digital output channel to the daq task.
    };

  }

}
