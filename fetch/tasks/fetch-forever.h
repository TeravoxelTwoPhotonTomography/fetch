#pragma once

#include "task.h"
#include "agent.h"

namespace fetch
{ namespace task
  { namespace digitizer
    {
      template<class TPixel>
        class FetchForever
        { public:
          unsigned int config(Digitizer *d);
          unsigned int run(Digitizer *d);
        };
    }
  }
}
