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
#include "..\tasks\Video.h"
#include "..\devices\microscope.h"
#include "microscope-interaction.h"

namespace fetch
{ namespace task
  { namespace microscope
    {   
      // Upcasting
      unsigned int Interaction::config(Agent *d) {return config(dynamic_cast<device::Microscope*>(d));}
      unsigned int Interaction::run   (Agent *d) {return run   (dynamic_cast<device::Microscope*>(d));}   
      
      
      //
      // Implementation
      //
      
      unsigned int Interaction::config(device::Microscope *agent)
      { static task::scanner::Video<i16> focus;
      
        //Assemble pipeline here
	      Agent *cur;
	      cur = &agent->scanner;
	      cur =  agent->pixel_averager.apply(cur);
	      cur =  agent->frame_averager.apply(cur);
	      cur =  agent->inverter.apply(cur);
	      cur =  agent->cast_to_i16.apply(cur);
	      cur =  agent->wrap.apply(cur);
	      cur =  agent->trash.apply(cur);
	      
	      agent->scanner.arm_nowait(&focus,INFINITE);
	      //agent->scanner.run_nonblocking();
	      
        return 1; //success
      }            
      
      unsigned int Interaction::run(device::Microscope *agent) {return 1;} // do nothing
    }
  }
}
