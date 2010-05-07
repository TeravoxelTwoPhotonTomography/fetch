/*
 * MicroscopeStateMenu.cpp
 *
 *  Created on: Apr 29, 2010
 *      Author: Nathan
 */

#include "stdafx.h"
#include "MicroscopeStateMenu.h"
#include "../devices/microscope.h"

namespace fetch {

	namespace ui {
    MicroscopeStateMenu::_t_task_table
    MicroscopeStateMenu::__tasktable[] =
      {
        { "&Interactive", -1, new task::microscope::Interaction },
        { NULL, NULL, NULL }, };
	          
		MicroscopeStateMenu::MicroscopeStateMenu(device::Microscope *scope)
		                    :AgentStateMenu("&Microscope",scope)
		{ _task_table = this->__tasktable;
		  DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_MICROSCOPE_TASK_0, _instance_id);
		  _task_table[0].messageid = IDM_MICROSCOPE_TASK_0;
		}

	}

}
