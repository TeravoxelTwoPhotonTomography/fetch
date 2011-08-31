#include "devices/Vibratome.h"
#include "tasks/Vibratome.h"

namespace fetch {
namespace task {
namespace vibratome {

  unsigned int Activate::run(device::Vibratome* dc)
  { dc->start();
    while(!dc->_agent->is_stopping());
    dc->stop();
    return 0;
  }

  unsigned int Cut::run(device::Vibratome* dc)
  { dc->start();
    while(!dc->_agent->is_stopping());
    dc->stop();
    return 0;
  }
}}} // end fetch::task::vibratome

