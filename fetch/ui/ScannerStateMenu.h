/*
 * ScannerStateMenu.h
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#ifndef SCANNERSTATEMENU_H_
#define SCANNERSTATEMENU_H_

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
      DECLARE_USER_MESSAGE_NON_STATIC( IDM_SCANNER2D_TASK_0);
      DECLARE_USER_MESSAGE_NON_STATIC( IDM_SCANNER2D_TASK_1);

      static _t_task_table _task_table[] =
        {
          { "Video  8-bit", IDM_SCANNER2D_TASK_0, task::scanner2d::Video<i8>() },
          { "Video 16-bit", IDM_SCANNER2D_TASK_1, task::scanner2d::Video<i16>() },
          { NULL, NULL, NULL }, };
    };

  }

}

#endif /* SCANNERSTATEMENU_H_ */
