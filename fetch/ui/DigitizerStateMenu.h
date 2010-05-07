/*
 * DigitizerStateMenu.h
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
 /*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#pragma once

#include "../stdafx.h"
#include "AgentStateMenu.h"
#include "../devices/digitizer.h"
#include "../tasks/fetch-forever.h"

namespace fetch
{

  namespace ui
  {

    class DigitizerStateMenu : public fetch::ui::AgentStateMenu
    {
    public:
      DigitizerStateMenu(device::Digitizer *digitizer);

    protected:
      UINT IDM_DIGITIZER_TASK_0;
      UINT IDM_DIGITIZER_TASK_1;

      static _t_task_table __task_table[];
    };

  }

}

