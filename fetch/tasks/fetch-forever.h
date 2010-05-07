#pragma once

#include "../task.h"
#include "../agent.h"
#include "../devices/Digitizer.h"

namespace fetch
{ namespace task
  { namespace digitizer
    {
      //typedef UpcastTask<device::Digitizer> DigitizerTask;
      typedef Task DigitizerTask;
    
      template<class TPixel>
        class FetchForever : public DigitizerTask
        { public:
          unsigned int config (Agent *d);
          unsigned int run    (Agent *d);
          
          unsigned int config(device::Digitizer *d);
          unsigned int run   (device::Digitizer *d);
        };
    }
  }
}
