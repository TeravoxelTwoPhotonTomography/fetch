/*
 * microscope-interaction.cpp
 *
 * Author: Nathan Clack <clackn@janelia.hhmi.org>
 *   Date: Apr 28, 2010
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma once
#include "stdafx.h"
#include "..\task.h"
#include "..\devices\microscope.h"
#include "microscope-interaction.h"

namespace fetch
{ namespace task
  { namespace microscope
    {      
      unsigned int Interaction::config(device::Microscope *agent)
      { //Assemble pipeline here
	      Agent *cur;
	      cur = &agent->scanner;
	      cur =  agent->pixel_averager.apply(cur);
	      cur =  agent->trash.apply(cur);
        return 1; //success
      }            
      
      unsigned int Interaction::run(device::Microscope *agent) {return 1;} // do nothing
    }
  }
}
