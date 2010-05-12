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
#include "../devices/scanner3d.h"
#include "../tasks/Video.h"
#include "AgentStateMenu.h"

namespace fetch
{

  namespace ui
  {

    class ScannerStateMenu : public fetch::ui::AgentStateMenu
    {
    public:
      ScannerStateMenu(device::Scanner3D *scanner);
    protected:
      UINT IDM_SCANNER2D_TASK_0;
      UINT IDM_SCANNER2D_TASK_1;
      UINT IDM_SCANNER2D_TASK_2;
      UINT IDM_SCANNER2D_TASK_3;

      static _t_task_table __task_table[];
    };

  }

}

#endif /* SCANNERSTATEMENU_H_ */
