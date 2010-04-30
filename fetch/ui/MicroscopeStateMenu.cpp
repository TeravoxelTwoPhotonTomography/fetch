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

		MicroscopeStateMenu::MicroscopeStateMenu(device::Microscope *scope)
		                    :AgentStateMenu("&Microscope",scope)
		{
		  DEFINE_USER_MESSAGE_INT__NON_STATIC( IDM_MICROSCOPE_TASK_0, _instance_id);
		  _task_table[0].messageid = IDM_MICROSCOPE_TASK_0;
		}

	}

}
