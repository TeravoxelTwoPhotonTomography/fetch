/*
 * ScannerStateMenu.h
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

#ifndef SCANNERSTATEMENU_H_
#define SCANNERSTATEMENU_H_

#include "../stdafx.h"
#include "../devices/scanner2d.h"
#include "../tasks/Video.h"
#include "AgentStateMenu.h"

namespace fetch
{

  namespace ui
  {

    class ScannerStateMenu : public fetch::ui::AgentStateMenu
    {
    public:
      ScannerStateMenu(device::Scanner2D *scanner);
    protected:
      UINT IDM_SCANNER2D_TASK_0;
      UINT IDM_SCANNER2D_TASK_1;

      static _t_task_table _task_table[];
    };
    
    ScannerStateMenu::_t_task_table ScannerStateMenu::_task_table[] =
        {
          { "Video  8-bit", -1, new task::scanner::Video<i8>() },
          { "Video 16-bit", -1, new task::scanner::Video<i16>() },
          { NULL, NULL, NULL }, };

  }

}

#endif /* SCANNERSTATEMENU_H_ */
