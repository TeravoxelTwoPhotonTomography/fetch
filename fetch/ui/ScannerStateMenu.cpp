/*
 * ScannerStateMenu.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "ScannerStateMenu.h"

namespace fetch
{

  namespace ui
  {

    ScannerStateMenu::ScannerStateMenu()
    {
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_SCANNER2D_TASK_0, _instance_id);
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_SCANNER2D_TASK_1, _instance_id);
    }

  }

}
