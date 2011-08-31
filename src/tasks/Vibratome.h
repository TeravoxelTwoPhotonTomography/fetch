#pragma once

#include "task.h"

namespace fetch{

// Forward declarations
namespace device { class Vibratome; }

namespace task {
namespace vibratome {

  // Just run the vibratome
  class Activate : public TTask<device::Vibratome>
  { public:
    virtual unsigned int run(device::Vibratome* dc);
  };

  // Turn the vibratome on and feed using the stage.
  class Cut : public TTask<device::Vibratome>
  { public:
    virtual unsigned int run(device::Vibratome* dc);
  };

}}} // fetch::task::vibratome
