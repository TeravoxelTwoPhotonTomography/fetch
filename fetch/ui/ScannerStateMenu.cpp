/*
 * ScannerStateMenu.cpp
 *
 *  Created on: Apr 20, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */

#include "stdafx.h"
#include "ScannerStateMenu.h"
#include "../tasks/StackAcquisition.h"

namespace fetch
{

  namespace ui
  { 
    
    ScannerStateMenu::_t_task_table ScannerStateMenu::__task_table[] =
        {
          { "Video  8-bit", -1, new task::scanner::Video<i8>() },
          { "Video 16-bit", -1, new task::scanner::Video<i16>() },
          { "Stack  8-bit", -1, new task::scanner::ScanStack<i8>() },
          { "Stack 16-bit", -1, new task::scanner::ScanStack<i16>() },
          { NULL, NULL, NULL }, };

    ScannerStateMenu::ScannerStateMenu(device::Scanner3D *scanner)
                     :AgentStateMenu("&Scanner",scanner)                      
    { _task_table = __task_table;
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_SCANNER2D_TASK_0, _instance_id);
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_SCANNER2D_TASK_1, _instance_id);
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_SCANNER2D_TASK_2, _instance_id);
      DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_SCANNER2D_TASK_3, _instance_id);
      _task_table[0].messageid = IDM_SCANNER2D_TASK_0;
      _task_table[1].messageid = IDM_SCANNER2D_TASK_1;
      _task_table[2].messageid = IDM_SCANNER2D_TASK_2;
      _task_table[3].messageid = IDM_SCANNER2D_TASK_3;
    }

  }

}
