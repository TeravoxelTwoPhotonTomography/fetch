/*
 * MicroscopeStateMenu.h
 *
 *  Created on: Apr 29, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

#pragma once

#include "AgentStateMenu.h"
#include "../devices/microscope.h"
#include "../tasks/microscope-interaction.h"

namespace fetch {

	namespace ui {

		class MicroscopeStateMenu: public fetch::ui::AgentStateMenu {
		public:
			MicroscopeStateMenu(device::Microscope *scope);
	    protected:
	      UINT IDM_MICROSCOPE_TASK_0;
	    };

		MicroscopeStateMenu::_t_task_table MicroscopeStateMenu::_task_table[] =
	        {
	          { "&Interactive", -1, new task::microscope::Interaction },
	          { NULL, NULL, NULL }, };

	}
}

