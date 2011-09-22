#pragma once

#include "task.h"

namespace fetch{

// Forward declarations
namespace device { 
  class Vibratome; 
  class Microscope;
}

namespace task {
namespace vibratome {

  // Just run the vibratome
  class Activate : public TTask<device::Vibratome>
  { public:
    virtual unsigned int run(device::Vibratome* dc);
  };

} // end fetch::task::vibratome

namespace microscope {  

  // Turn the vibratome on and feed using the stage.
  class Cut : public TTask<device::Microscope>
  { public:
    virtual unsigned int config(device::Microscope* dc);
    virtual unsigned int run(device::Microscope* dc);
  };

} // fetch::task::microscope

}}
