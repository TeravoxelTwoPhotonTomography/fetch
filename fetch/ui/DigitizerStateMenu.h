/*
 * DigitizerStateMenu.h
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#pragma once

#include "AgentStateMenu.h"
#include "devices/digitizer.h"
#include "tasks/fetch-forever.h"

namespace fetch
{

  namespace ui
  {

    class DigitizerStateMenu : public fetch::ui::AgentStateMenu
    {
    public:
      DigitizerStateMenu(device::Digitizer *digitizer);

    protected:
      DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER_TASK_0);
      DECLARE_USER_MESSAGE_NON_STATIC( IDM_DIGITIZER_TASK_1);

      static _t_task_table _task_table[] =
        {
          { "Fetch Forever  8-bit", IDM_DIGITIZER_TASK_0, task::digitizer::FetchForever<i8>() },
          { "Fetch Forever 16-bit", IDM_DIGITIZER_TASK_1, task::digitizer::FetchForever<i16>() },
          { NULL, NULL, NULL }, };
    };

  }

}

