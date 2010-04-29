/*
 * ScannerStateMenu.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "stdafx.h"
#include "ScannerStateMenu.h"

namespace fetch
{

  namespace ui
  {

    ScannerStateMenu::ScannerStateMenu(device::Scanner2D *scanner)
                     :AgentStateMenu("&Scanner",scanner)
    {
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_SCANNER2D_TASK_0, _instance_id);
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_SCANNER2D_TASK_1, _instance_id);
      _task_table[0].messageid = IDM_SCANNER2D_TASK_0;
      _task_table[1].messageid = IDM_SCANNER2D_TASK_1;
    }

  }

}
