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
    //////////////////////////////////////////////////////////////////////////
    template<class Tcfg>
    class IDigitizer : public IConfigurableDevice<Tcfg>
    {
    public:
      IDigitizer(Agent* agent) : IConfigurableDevice(agent) {}
      IDigitizer(Agent* agent, Config* cfg) : IConfigurableDevice(agent,cfg) {}
    };

    //////////////////////////////////////////////////////////////////////////
    class NIScopeDigitizer : public IDigitizer<cfg::device::Digitizer>
    {
    public:
      typedef cfg::device::Digitizer          Config;
      typedef cfg::device::Digitizer_Channel  Channel_Config;

      NIScopeDigitizer(Agent *agent);      
      NIScopeDigitizer(Agent *agent, Config *cfg); // Configuration references cfg
      //NIScopeDigitizer(size_t nbuf, size_t nbytes_per_frame, size_t nwfm); // Inputs determine how output queues are initially allocated


      ~NIScopeDigitizer();

      unsigned int attach(void);
      unsigned int detach(void);

    public:

      ViSession _vi;

    private:
      void __common_setup();
    };

    //////////////////////////////////////////////////////////////////////////
    class SimulatedDigitizer : public IDigitizer<int>
    {
    public:
      SimulatedDigitizer(Agent *agent) : IDigitizer<int>(agent) {}
    };
  }
} // namespace fetch
