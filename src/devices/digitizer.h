/*
 * Digitizer.h
 *
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 20, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/*
 * Notes
 * -----
 *
 * In niScope, a waveform is a single channel for a single record.  A
 * multi-record acquisition is used when one needs fast retriggering.  The data
 * in a record is formated:
 *
 *   [--- chan 0 ---] [--- chan 1 ---] ... [--- chan N ---] : record k
 *
 * For a multirecord acquisition:
 *
 *   [--- record 1 ---][--- record 2 ---] ...
 *
 * So, we have this identity:
 *
 *   #waveforms = #records x #channels
 */
#pragma once

#include "../agent.h"
#include "../util/util-niscope.h"
#include "digitizer.pb.h"
#include "object.h"

#define DIGITIZER_BUFFER_NUM_FRAMES       4        // must be a power of two
#define DIGITIZER_DEFAULT_TIMEOUT         INFINITE // ms

namespace fetch
{
  namespace device
  {
    class Digitizer
      : public virtual Agent,
        public Configurable<cfg::device::Digitizer>
    {
    public:
      typedef cfg::device::Digitizer          Config;
      typedef cfg::device::Digitizer_Channel  Channel_Config;

      Digitizer();      
      Digitizer(Config *cfg); // Configuration references cfg
      Digitizer(size_t nbuf, size_t nbytes_per_frame, size_t nwfm); // Inputs determine how output queues are initially allocated


      ~Digitizer();

      unsigned int attach(void);
      unsigned int detach(void);
      //unsigned int is_attached(void);

    public:

      ViSession vi;

    private:
      void __common_setup();
    };
  }
} // namespace fetch
