/*
 * DigitizerStateMenu.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "DigitizerStateMenu.h"

namespace fetch
{

  namespace ui
  {

    DigitizerStateMenu::DigitizerStateMenu(device::Digitizer *digitizer)
                       :AgentStateMenu(digitizer)
    {
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_TASK_0, _instance_id);
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_TASK_1, _instance_id);
    }


  }

}
