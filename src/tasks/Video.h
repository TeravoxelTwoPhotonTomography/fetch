/*
 * Video.h
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#pragma once
#include "config.h"
#include "task.h"
#include "devices/scanner2D.h"
#include "devices/scanner3D.h"
#include "agent.h"

namespace fetch
{

  namespace task
  {
    namespace scanner
    {
      template<class TPixel>
      class Video : public Task, public IUpdateable
      {
        public:         
          //unsigned int run    (device::Scanner3D *d);
          unsigned int run    (IDevice *d);
          unsigned int config (IDevice *d);
          unsigned int update (IDevice *d);
          
          unsigned int _run    (device::IScanner *d);          
          unsigned int _config (device::IScanner *d);          
          unsigned int _update (device::IScanner *d);

          unsigned int run_niscope(device::IScanner *d);
          unsigned int run_alazar(device::IScanner *d);
          unsigned int run_simulated(device::IScanner *d);

        protected:
      };

#ifdef HAVE_NISCOPE
      template<typename T>
      Frame_With_Interleaved_Lines
        _describe_actual_frame_niscope(device::NIScopeDigitizer *dig, ViInt32 nscans, ViInt32 *precordsize, ViInt32 *pnwfm);
#endif
    }
  }
}
