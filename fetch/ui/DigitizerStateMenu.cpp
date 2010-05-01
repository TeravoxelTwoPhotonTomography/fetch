/*
 * DigitizerStateMenu.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "stdafx.h"
#include "DigitizerStateMenu.h"

namespace fetch
{

  namespace ui
  {
    
    DigitizerStateMenu::_t_task_table DigitizerStateMenu::__task_table[] =
        {
          { "Fetch Forever  8-bit", -1, new task::digitizer::FetchForever<i8>() },
          { "Fetch Forever 16-bit", -1, new task::digitizer::FetchForever<i16>() },
          { NULL, NULL, NULL }, };

    DigitizerStateMenu::DigitizerStateMenu(device::Digitizer *digitizer)
                       :AgentStateMenu("&Digitizer",digitizer)
                         
    { _task_table = __task_table;
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_TASK_0, _instance_id);
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_DIGITIZER_TASK_1, _instance_id);
      _task_table[0].messageid = IDM_DIGITIZER_TASK_0;
      _task_table[1].messageid = IDM_DIGITIZER_TASK_1;
    }


  }

}
