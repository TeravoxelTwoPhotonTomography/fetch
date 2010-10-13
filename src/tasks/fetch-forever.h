#pragma once

#include "task.h"
#include "agent.h"
#include "devices/digitizer.h"

namespace fetch
{ namespace task
  { namespace digitizer
    {
      //typedef UpcastTask<device::Digitizer> DigitizerTask;
      typedef Task DigitizerTask;
    
      template<class TPixel>
        class FetchForever : public DigitizerTask
        { public:
          unsigned int config (IDevice *d);
          unsigned int run    (IDevice *d);
          
          unsigned int config(device::NIScopeDigitizer *d);
          unsigned int run   (device::NIScopeDigitizer *d);
        };
    }
  }
}
